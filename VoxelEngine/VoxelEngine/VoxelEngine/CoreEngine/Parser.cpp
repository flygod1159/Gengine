#include "Parser.h"
#include "GAssert.h"
#include "CVarInternal.h"

enum class CmdType
{
  INVALID,
  FLOAT,
  STRING,
  IDENTIFIER, // command or cvar identifier
};

CmdParser::CmdParser(const char* command)
  : cmd(command)
{
  // trim whitespace from beginning and end of string
  cmd.erase(0, cmd.find_first_not_of(" \n\r\t"));
  cmd.erase(cmd.find_last_not_of(" \n\r\t") + 1);
}

// world's worst type parser
// probably better than nothing
CmdAtom CmdParser::NextAtom()
{
  if (!Valid())
  {
    return ParseError{ .where = current, .what = "Empty command" };
  }

  CmdType type = CmdType::INVALID;

  // determine type by first character
  if (cmd[current] == '"')
  {
    type = CmdType::STRING;
    current++;
  }
  else if (std::isalpha(cmd[current]) || cmd[current] == '_')
  {
    type = CmdType::IDENTIFIER;
  }
  else if (std::isdigit(cmd[current]) || cmd[current] == '-' || cmd[current] == '.')
  {
    type = CmdType::FLOAT;
  }
  else
  {
    current = cmd.size();
    return ParseError{ .where = current, .what = "Token begins with invalid character" };
  }

  std::string atom;
  bool escapeNextChar{ false };
  for (; Valid();)
  {
    char c = cmd[current];
    current++;

    if (std::isblank(c) && type != CmdType::STRING)
    {
      break;
    }

    if (type == CmdType::STRING)
    {
      // end string with un-escaped quotation mark
      if (c == '"' && !escapeNextChar)
      {
        break;
      }

      // unescaped backslash will escape next character, how confusing!
      if (c == '\\' && !escapeNextChar)
      {
        escapeNextChar = true;
        continue;
      }

      escapeNextChar = false;
    }

    if (type == CmdType::IDENTIFIER)
    {
      if (!(std::isalnum(c) || c == '.' || c == '_'))
      {
        current = cmd.size();
        return ParseError{ .where = current, .what = "Invalid character in identifier" };
      }
    }

    atom.push_back(c);
  }

  // skip any extra whitespace there may be between atoms
  current = cmd.find_first_not_of(" \n\r\t", current);
  
  switch (type)
  {
  case CmdType::FLOAT:
  {
    size_t read{};
    float f = std::stod(atom, &read);
    if (read != atom.size())
    {
      return ParseError{ .where = current, .what = "Tried to read float, but failed" };
    }
    return f;
  }
  case CmdType::STRING:
    return atom;
  case CmdType::IDENTIFIER:
    return Identifier{ .name = atom };
  default:
    return ParseError{ .where = current, .what = "Could not determine type" };
  }

  // unreachable
}
