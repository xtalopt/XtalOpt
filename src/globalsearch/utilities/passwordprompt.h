// Written by Patrick S. Avery -- 2015

#ifndef PASSWORD_PROMPT_H
#define PASSWORD_PROMPT_H

class PasswordPrompt
{
public:
  static std::string getPassword(
    const std::string& prompt = "Enter password: ");
};

#endif // PASSWORD_PROMPT_H
