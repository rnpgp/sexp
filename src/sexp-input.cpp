/**
 *
 * Copyright (c) 2022, [Ribose Inc](https://www.ribose.com).
 * All rights reserved.
 * This file is a part of RNP sexp library
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other matrials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Original copyright
 *
 * SEXP implementation code sexp-input.c
 * Ron Rivest
 * 7/21/1997
 */

#include <sexp/sexp-error.h>
#include <sexp/sexp.h>

namespace sexp {

/**************************************/
/* CHARACTER ROUTINES AND DEFINITIONS */
/**************************************/
bool sexp_char_defs::initialized = false;
char sexp_char_defs::upper[256];       /* upper[c] is upper case version of c */
bool sexp_char_defs::whitespace[256];  /* whitespace[c] is nonzero if c is whitespace */
bool sexp_char_defs::decdigit[256];    /* decdigit[c] is nonzero if c is a dec digit */
char sexp_char_defs::decvalue[256];    /* decvalue[c] is value of c as dec digit */
bool sexp_char_defs::hexdigit[256];    /* hexdigit[c] is nonzero if c is a hex digit */
char sexp_char_defs::hexvalue[256];    /* hexvalue[c] is value of c as a hex digit */
bool sexp_char_defs::base64digit[256]; /* base64char[c] is nonzero if c is base64 digit */
char sexp_char_defs::base64value[256]; /* base64value[c] is value of c as base64 digit */
bool sexp_char_defs::tokenchar[256];   /* tokenchar[c] is true if c can be in a token */
bool sexp_char_defs::alpha[256];       /* alpha[c] is true if c is alphabetic A-Z a-z */

/*
 * sexp_char_defs::initializeCharacterTables
 * initializes all of the above arrays
 */
void sexp_char_defs::initializeCharacterTables() {
  int i;
  for (i = 0; i < 256; i++) upper[i] = i;
  for (i = 'a'; i <= 'z'; i++) upper[i] = i - 'a' + 'A';
  for (i = 0; i <= 255; i++) {
    alpha[i] = base64digit[i] = decdigit[i] =
    hexdigit[i] = whitespace[i] = false;
  }
  whitespace[' '] = whitespace['\n'] = whitespace['\t'] = true;
  whitespace['\v'] = whitespace['\r'] = whitespace['\f'] = true;
  for (i = '0'; i <= '9'; i++) {
    base64digit[i] = decdigit[i] = hexdigit[i] = true;
    decvalue[i] = hexvalue[i] = i - '0';
    base64value[i] = (i - '0') + 52;
  }
  for (i = 'a'; i <= 'f'; i++) {
    hexdigit[i] = hexdigit[upper[i]] = true;
    hexvalue[i] = hexvalue[upper[i]] = i - 'a' + 10;
  }
  for (i = 'a'; i <= 'z'; i++) {
    base64digit[i] = base64digit[upper[i]] = true;
    alpha[i] = alpha[upper[i]] = true;
    base64value[i] = i - 'a' + 26;
    base64value[upper[i]] = i - 'a';
  }
  base64digit['+'] = base64digit['/'] = true;
  base64value['+'] = 62;
  base64value['/'] = 63;
  base64value['='] = 0;
  for (i = 0; i < 255; i++) tokenchar[i] = false;
  for (i = 'a'; i <= 'z'; i++) tokenchar[i] = tokenchar[upper[i]] = true;
  for (i = '0'; i <= '9'; i++) tokenchar[i] = true;
  tokenchar['-'] = true;
  tokenchar['.'] = true;
  tokenchar['/'] = true;
  tokenchar['_'] = true;
  tokenchar[':'] = true;
  tokenchar['*'] = true;
  tokenchar['+'] = true;
  tokenchar['='] = true;
}

/*
 * sexp_char_defs::isWhiteSpace(c)
 * Returns TRUE if c is a whitespace character (space, tab, etc. ).
 */
bool sexp_char_defs::isWhiteSpace(int c) { return ((c >= 0 && c <= 255) && whitespace[c]); }

/*
 * sexp_char_defs::isDecDigit(c)
 * Returns TRUE if c is a decimal digit.
 */
bool sexp_char_defs::isDecDigit(int c) { return ((c >= 0 && c <= 255) && decdigit[c]); }

/*
 * sexpInputStream::isHexDigit(c)
 * Returns TRUE if c is a hexadecimal digit.
 */
bool sexp_char_defs::isHexDigit(int c) { return ((c >= 0 && c <= 255) && hexdigit[c]); }

/*
 * sexp_char_defs::isBase64Digit(c)
 * returns TRUE if c is a base64 digit A-Z,a-Z,0-9,+,/
 */
bool sexp_char_defs::isBase64Digit(int c) { return ((c >= 0 && c <= 255) && base64digit[c]); }

/*
 * sexp_char_defs::isTokenChar(c)
 * Returns TRUE if c is allowed in a token
 */
bool sexp_char_defs::isTokenChar(int c) { return ((c >= 0 && c <= 255) && tokenchar[c]); }

/*
 * sexp_char_defs::isAlpha(c)
 * Returns TRUE if c is alphabetic
 */
bool sexp_char_defs::isAlpha(int c) { return ((c >= 0 && c <= 255) && alpha[c]); }

/*
 * newSexpInputStream()
 * Creates and initializes a new sexpInputStream object.
 * (Prefixes stream with one blank, and initializes stream
 *  so that it reads from standard input.)
 */

sexpInputStream::sexpInputStream(std::istream *i)
    : inputFile(i), nextChar(' '), count(-1), byteSize(8), bits(0), nBits(0) {
    }

/*
 * sexpInputStream::setByteSize(newByteSize)
 */
sexpInputStream *sexpInputStream::setByteSize(size_t newByteSize) {
  byteSize = newByteSize;
  nBits = 0;
  bits = 0;
  return this;
}

/*
 * sexpInputStream::getChar()
 * This is one possible character input routine for an input stream.
 * (This version uses the standard input stream.)
 * getChar places next 8-bit character into is->nextChar.
 * It also updates the count of number of 8-bit characters read.
 * The value EOF is obtained when no more input is available.
 * This code handles 4-bit/6-bit/8-bit channels.
 */
sexpInputStream *sexpInputStream::getChar(void) {
  int c;
  if (nextChar == EOF) {
    byteSize = 8;
    return this;
  }

  while (true) {
    c = nextChar = inputFile->get();
    if (c == EOF)
      return this;
    if ((byteSize == 6 && (c == '|' || c == '}')) ||
        (byteSize == 4 && (c == '#'))) {
      // end of region reached; return terminating character, after checking for
      // unused bits
      if (nBits > 0 && (((1 << nBits) - 1) & bits) != 0) {
        ErrorMessage(sexp_exception::warning,
                     "%d-bit region ended with %d unused bits left-over",
                     byteSize, nBits, count);
      }
      return setByteSize(8);
    } else if (byteSize != 8 && isWhiteSpace(c))
      ; /* ignore white space in hex and base64 regions */
    else if (byteSize == 6 && c == '=')
      ; /* ignore equals signs in base64 regions */
    else if (byteSize == 8) {
      count++;
      return this;
    } else if (byteSize < 8) {
      bits = bits << byteSize;
      nBits += byteSize;
      if (byteSize == 6 && isBase64Digit(c))
        bits = bits | base64value[c];
      else if (byteSize == 4 && isHexDigit(c))
        bits = bits | hexvalue[c];
      else {
        ErrorMessage(sexp_exception::error,
                     "character '%c' found in %d-bit coding region", nextChar,
                     byteSize, count);
      }
      if (nBits >= 8) {
        nextChar = (bits >> (nBits - 8)) & 0xFF;
        nBits -= 8;
        count++;
        return this;
      }
    }
  }
}

/*
 * sexpInputStream::skipWhiteSpace
 * Skip over any white space on the given sexpInputStream.
 */
sexpInputStream *sexpInputStream::skipWhiteSpace(void) {
  while (isWhiteSpace(nextChar)) getChar();
  return this;
}

/*
 * sexpInputStream::skipChar(c)
 * Skip the following input character on input stream is, if it is
 * equal to the character c.  If it is not equal, then an error occurs.
 */
sexpInputStream *sexpInputStream::skipChar(int c) {
  if (nextChar != c)
    ErrorMessage(sexp_exception::error,
                 "character %x (hex) found where %c (char) expected", nextChar,
                 c, count);
  return getChar();
}

/*
 * sexpInputStream::scanToken(ss)
 * scan one or more characters into simple string ss as a token.
 */
void sexpInputStream::scanToken(sexpSimpleString *ss) {
  skipWhiteSpace();
  while (isTokenChar(nextChar)) {
    ss->append(nextChar);
    getChar();
  }
  return;
}

/*
 * sexpInputStream::scanToEOF(void)
 * scan one or more characters (until EOF reached)
 * return an object that is just that string
 */
sexpObject* sexpInputStream::scanToEOF(void) {
  sexpSimpleString *ss = new sexpSimpleString();
  sexpString *s = new sexpString();
  s->setString(ss);
  skipWhiteSpace();
  while (nextChar != EOF) {
    ss->append(nextChar);
    getChar();
  }
  return s;
}

/*
 * scanDecimal(is)
 * returns long integer that is value of decimal number
 */
int sexpInputStream::scanDecimal(void) {
  int value = 0;
  size_t i = 0;
  while (isDecDigit(nextChar)) {
    value = value * 10 + decvalue[nextChar];
    getChar();
    if (i++ > 8)
      ErrorMessage(sexp_exception::error, "Decimal number %d... too long.",
                   (int)value, 0, count);
  }
  return (value);
}

/*
 * sexpInputStream::scanVerbatimString(is,ss,length)
 * Reads verbatim string of given length into simple string ss.
 */
void sexpInputStream::scanVerbatimString(sexpSimpleString *ss, int length) {
  skipWhiteSpace()->skipChar(':');
  if (length == -1L) /* no length was specified */
    ErrorMessage(sexp_exception::error,
                 "Verbatim string had no declared length.", 0, 0, count);
  for (int i = 0; i < length; i++) {
    ss->append(nextChar);
    getChar();
  }
  return;
}

/*
 * sexpInputStream::scanQuotedString(ss,length)
 * Reads quoted string of given length into simple string ss.
 * Handles ordinary C escapes.
 * If of indefinite length, length is -1.
 */
void sexpInputStream::scanQuotedString(sexpSimpleString *ss, int length) {
  int c;
  skipChar('"');
  while (length == -1 || ss->length() <= length) {
    if (nextChar == '\"') {
      if (length == -1 || (ss->length() == length)) {
        skipChar('\"');
        return;
      } else
        ErrorMessage(sexp_exception::error,
                     "Quoted string ended too early. Declared length was %d",
                     (int)length, 0, count);
    } else if (nextChar == '\\') /* handle escape sequence */
    {
      getChar();
      c = nextChar;
      if (c == 'b')         ss->append('\b');
      else if (c == 't')    ss->append('\t');
      else if (c == 'v')    ss->append('\v');
      else if (c == 'n')    ss->append('\n');
      else if (c == 'f')    ss->append('\f');
      else if (c == 'r')    ss->append('\r');
      else if (c == '\"')   ss->append('\"');
      else if (c == '\'')   ss->append('\'');
      else if (c == '\\')   ss->append('\\');
      else if (c >= '0' && c <= '7') { /* octal number */
        int j, val;
        val = 0;
        for (j = 0; j < 3; j++) {
          if (c >= '0' && c <= '7') {
            val = ((val << 3) | (c - '0'));
            if (j < 2) {
              getChar();
              c = nextChar;
            }
          } else
            ErrorMessage(sexp_exception::error,
                         "Octal character \\%o... too short", val, 0,
                         count);
        }
        if (val > 255)
          ErrorMessage(sexp_exception::error,
                       "Octal character \\%o... too big", val, 0, count);
        ss->append(val);
      } else if (c == 'x') /* hexadecimal number */
      {
        int j, val;
        val = 0;
        getChar();
        c = nextChar;
        for (j = 0; j < 2; j++) {
          if (isHexDigit(c)) {
            val = ((val << 4) | hexvalue[c]);
            if (j < 1) {
              getChar();
              c = nextChar;
            }
          } else
            ErrorMessage(sexp_exception::error,
                         "Hex character \\x%x... too short", val, 0,
                         count);
        }
        ss->append(val);
      } else if (c == '\n') /* ignore backslash line feed */
      { /* also ignore following carriage-return if present */
        getChar();
        if (nextChar != '\r')
          continue;
        ;
      } else if (c == '\r') /* ignore backslash carriage-return */
      {                     /* also ignore following linefeed if present */
        getChar();
        if (nextChar != '\n')
          continue;
      } else
        ErrorMessage(sexp_exception::warning,
                     "Escape character \\%c... unknown.", c, 0, count);
    } /* end of handling escape sequence */
    else if (nextChar == EOF) {
      ErrorMessage(sexp_exception::error, "unxpected end of file", 0, 0,
                   count);
    } else {
      ss->append(nextChar);
    }
    getChar();
  } /* end of main while loop */
  return;
}

/*
 * scanHexString(ss,length)
 * Reads hexadecimal string into simple string ss.
 * String is of given length result, or length = -1 if indefinite length.
 */
void sexpInputStream::scanHexString(sexpSimpleString *ss, int length) {
  setByteSize(4)->skipChar('#');
  while (nextChar != EOF && (nextChar != '#' || getByteSize() == 4)) {
    ss->append(nextChar);
    getChar();
  }
  skipChar('#');
  if (ss->length() != length && length >= 0)
    ErrorMessage(sexp_exception::warning,
                 "Hex string has length %d different than declared length %d",
                 ss->length(), length, count);
}

/*
 * sexpInputStream::scanBase64String(ss,length)
 * Reads base64 string into simple string ss.
 * String is of given length result, or length = -1 if indefinite length.
 */
void sexpInputStream::scanBase64String(sexpSimpleString *ss, int length) {
  setByteSize(6)->skipChar('|');
  while (nextChar != EOF && (nextChar != '|' || getByteSize() == 6)) {
    ss->append(nextChar);
    getChar();
  }
  skipChar('|');
  if (ss->length() != length && length >= 0)
    ErrorMessage(
        sexp_exception::warning,
        "Base64 string has length %d different than declared length %d",
        ss->length(), length, count);
}

/*
 * sexpInputStream::scanSimpleString(void)
 * Reads and returns a simple string from the input stream.
 * Determines type of simple string from the initial character, and
 * dispatches to appropriate routine based on that.
 */
sexpSimpleString* sexpInputStream::scanSimpleString(void) {
  int length;
  sexpSimpleString *ss = new sexpSimpleString;
  skipWhiteSpace();
  /* Note that it is important in the following code to test for token-ness
   * before checking the other cases, so that a token may begin with ":",
   * which would otherwise be treated as a verbatim string missing a length.
   */
  if (isTokenChar(nextChar) && !isDecDigit(nextChar)) {
    scanToken(ss);
  } else if (isDecDigit(nextChar) || nextChar == '\"' ||
             nextChar == '#' || nextChar == '|' ||
             nextChar == ':') {
    if (isDecDigit(nextChar))
      length = scanDecimal();
    else
      length = -1L;
    if (nextChar == '\"')
      scanQuotedString(ss, length);
    else if (nextChar == '#')
      scanHexString(ss, length);
    else if (nextChar == '|')
      scanBase64String(ss, length);
    else if (nextChar == ':')
      scanVerbatimString(ss, length);
  } else {
    const char *const msg = (nextChar == EOF)
                                ? "unxpected end of file"
                                : isprint(nextChar)
                                      ? "illegal character '%c' (%d decimal)"
                                      : "illegal character %d (decimal)";
    ErrorMessage(sexp_exception::error, msg, nextChar, nextChar, count);
  }
  if (ss->length() == 0)
    ErrorMessage(sexp_exception::warning, "Simple string has zero length", 0,
                 0, count);
  return ss;
}

/*
 * sexpInputStream::scanString(void)
 * Reads and returns a string [presentationhint]string from input stream.
 */
sexpString* sexpInputStream::scanString(void) {
  sexpString *s = new sexpString();
  if (nextChar == '[') { /* scan presentation hint */
    skipChar('[');
    s->setPresentationHint(scanSimpleString());
    skipWhiteSpace()->skipChar(']')->skipWhiteSpace();
  }
  s->setString(scanSimpleString());
  return (s);
}

/*
 * sexpInputStream::scanList(void)
 * Read and return a sexpList from the input stream.
 */
sexpList* sexpInputStream::scanList(void) {
  sexpList *list = new sexpList();
  skipChar('(')->skipWhiteSpace();
  if (nextChar == ')') {
    ; /* OK */
  } else {
    list->push_back(scanObject());
  }

  while (true) {
    skipWhiteSpace();
    if (nextChar == ')') { /* we just grabbed last element of list */
      skipChar(')');
      return list;
    } else {
      list->push_back(scanObject());
    }
  }
  return list;
}

/*
 * sexpInputStream::scanObject(void)
 * Reads and returns a sexpObject from the given input stream.
 */
sexpObject* sexpInputStream::scanObject(void) {
  sexpObject *object;
  skipWhiteSpace();
  if (nextChar == '{') {
    setByteSize(6)->skipChar('{');
    object = scanObject();
    skipChar('}');
  }
  else {
    if (nextChar == '(')
      object = scanList();
    else
      object = scanString();
  }
  return object;
}

}