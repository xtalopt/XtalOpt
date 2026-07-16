/**********************************************************************
  search_templates - The template-text keyword engine of SearchBase:
                     keyword registration, %keyword% interpretation,
                     and keyword help.
                     Per-step template STORAGE is kept in OptSteps.

  Copyright (C) 2010-2011 by David C. Lonie
  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <search/search.h>

#include <search/optimizer.h>
#include <common/output.h>
#include <search/queueinterface.h>
#include <search/structure.h>

#include <common/fileutils.h>

#include <QFile>
#include <QStringList>
#include <QTextStream>

namespace Search {
namespace {

void chopTrailingNewline(QString& text)
{
  if (text.endsWith('\n'))
    text.chop(1);
}

bool readableTemplateFile(const QString& filename, QString* error)
{
  if (!Common::isReadableFile(filename)) {
    if (error) {
      *error = QString("Template file is not readable: %1")
                 .arg(filename);
    }
    return false;
  }
  return true;
}

struct KeywordHelpRow
{
  QString keyword;
  QString description;
};

QString fallbackKeywordText(const QString& key)
{
  return "%" + key + "%";
}

QString continuationHelpText(const QString& line)
{
  QString text = line.trimmed();
  if (text.startsWith("["))
    text = "format: " + text;
  return text;
}

KeywordHelpRow parseKeywordHelp(const QString& key, const QString& help)
{
  KeywordHelpRow row;
  row.keyword = fallbackKeywordText(key);

  const QStringList lines = help.split('\n');
  if (lines.isEmpty()) {
    return row;
  }

  const QString firstLine = lines.first().trimmed();
  const int separator = firstLine.indexOf(" -- ");
  if (separator >= 0) {
    row.keyword = firstLine.left(separator).trimmed();
    row.description = firstLine.mid(separator + 4).trimmed();
  } else {
    row.description = firstLine;
  }

  if (row.keyword.isEmpty())
    row.keyword = fallbackKeywordText(key);

  for (int i = 1; i < lines.size(); ++i) {
    const QString extra = continuationHelpText(lines.at(i));
    if (extra.isEmpty())
      continue;
    if (!row.description.isEmpty())
      row.description += "; ";
    row.description += extra;
  }

  return row;
}

} // namespace

void SearchBase::registerKeyword(const QString& key, std::function<QString(Structure*)> handler,
                                 const QString& help)
{
  if (!m_keywordMap.contains(key))
    m_keywordOrder.append(key);
  m_keywordMap.insert(key, {handler, help});
}

QString SearchBase::interpretTemplate(const QString& str, Structure* structure)
{
  QStringList list = str.split("%");
  QString line;
  for (int line_ind = 0; line_ind < list.size(); line_ind++) {
    line = list.at(line_ind);
    interpretKeyword_base(line, structure);
    // Add other interpret keyword sections here if needed when subclassing
    if (line != list.at(line_ind)) // Line was a keyword
      list.replace(line_ind, line);
  }
  // Rejoin string
  QString ret = list.join("");
  ret += "\n";
  return ret;
}

void SearchBase::interpretKeyword_base(QString& line, Structure* structure)
{
  // Prefix-match keywords (cannot live in the exact-match hash)
  if (line.startsWith("filecontents:", Qt::CaseInsensitive)) {
    QString filename = line;
    filename.remove(0, QString("filecontents:").size());
    filename = filename.trimmed();
    QString err;
    if (!readableTemplateFile(filename, &err)) {
      Common::error(err);
      line = "";
      return;
    }
    QString rep;
    if (!Common::readFileToQString(filename, &rep))
      Common::error(QString("%1: could not open %2")
              .arg(__func__)
              .arg(filename));
    if (!rep.isEmpty()) {
      chopTrailingNewline(rep);
      line = rep;
    }
    return;
  }
  if (line.startsWith("copyfile:", Qt::CaseInsensitive)) {
    QString filename = line;
    filename.remove(0, QString("copyfile:").size());
    filename = filename.trimmed();
    QString err;
    if (!readableTemplateFile(filename, &err)) {
      Common::error(err);
      line = "";
      return;
    }
    structure->appendCopyFile(filename.toStdString());
    line = "";
    return;
  }

  // Find the keyword by exact match.
  auto it = m_keywordMap.constFind(line);
  if (it == m_keywordMap.constEnd())
    return;
  QString rep = it->handler(structure);
  if (!rep.isEmpty()) {
    chopTrailingNewline(rep);
    line = rep;
  }
}

QString SearchBase::getTemplateKeywordHelp_base()
{
  QList<KeywordHelpRow> rows;
  int keywordWidth = QString("Keyword").size();

  for (const QString& key : m_keywordOrder) {
    auto it = m_keywordMap.constFind(key);
    if (it == m_keywordMap.constEnd() || it->help.isEmpty())
      continue;

    const KeywordHelpRow row = parseKeywordHelp(key, it->help);
    keywordWidth = qMax(keywordWidth, row.keyword.size());
    rows.append(row);
  }

  QString text;
  QTextStream out(&text);
  out << "\nThe following keywords should be used instead of the indicated variable data:\n\n";
  out << QString("%1  %2\n")
           .arg(QString("Keyword"), -keywordWidth)
           .arg(QString("Description"));
  out << QString(keywordWidth, QChar('-')) << "  -----------\n";
  for (const auto& row : rows) {
    out << QString("%1  %2\n")
             .arg(row.keyword, -keywordWidth)
             .arg(row.description);
  }
  return text;
}

std::unique_ptr<QueueInterface> SearchBase::createQueueInterface(
  const std::string& queueName)
{
  return QueueInterface::createRegisteredQueueInterface(QString::fromStdString(queueName), this);
}

std::unique_ptr<Optimizer> SearchBase::createOptimizer(const std::string& optName)
{
  return Optimizer::createRegisteredOptimizer(QString::fromStdString(optName), this);
}

} // namespace Search
