/**********************************************************************
  registeredcreators - Register a named creator (optimizer or queue).

  Copyright (C) 2026 Samad Hajinazar

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef SEARCH_REGISTERED_CREATORS_H
#define SEARCH_REGISTERED_CREATORS_H

#include <QHash>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>

namespace Search {

// A list of named creator functions.
template <typename Product, typename Context>
class RegisteredCreators
{
public:
  typedef std::function<std::unique_ptr<Product>(Context*)> Creator;

  // Add or replace a creator.
  bool registerCreator(const QString& name, Creator creator)
  {
    const QString key = name.toLower();
    const bool inserted = !m_creators.contains(key);
    if (inserted)
      m_order.append(name);
    m_creators.insert(key, creator);
    return inserted;
  }

  // Return the registered names.
  QStringList names() const
  {
    return m_order;
  }

  // Create a product by name.
  std::unique_ptr<Product> create(const QString& name, Context* context) const
  {
    auto it = m_creators.find(name.toLower());
    if (it == m_creators.end())
      return std::unique_ptr<Product>();
    return it.value()(context);
  }

  // Return the shared list.
  static RegisteredCreators& shared()
  {
    static RegisteredCreators instance;
    return instance;
  }

private:
  QHash<QString, Creator> m_creators;
  QStringList m_order;
};

} // namespace Search

#endif // SEARCH_REGISTERED_CREATORS_H
