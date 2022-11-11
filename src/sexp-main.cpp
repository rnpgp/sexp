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
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Original copyright
 *
 * SEXP implementation code sexp-main.c
 * Ron Rivest
 * 6/29/1997
 **/

#include <sexp/sexp-error.h>
#include <sexp/sexp.h>

using namespace sexp;

const char *help =
"The program `sexp' reads, parses, and prints out S-expressions.\n"
" INPUT:\n"
"   Input is normally taken from stdin, but this can be changed:\n"
"      -i filename      -- takes input from file instead.\n"
"      -p               -- prompts user for console input\n"
"   Input is normally parsed, but this can be changed:\n"
"      -s               -- treat input up to EOF as a single string\n"
" CONTROL LOOP:\n"
"   The main routine typically reads one S-expression, prints it out again, \n"
"   and stops.  This may be modified:\n"
"      -x               -- execute main loop repeatedly until EOF\n"
" OUTPUT:\n"
"   Output is normally written to stdout, but this can be changed:\n"
"      -o filename      -- write output to file instead\n"
"   The output format is normally canonical, but this can be changed:\n"
"      -a               -- write output in advanced transport format\n"
"      -b               -- write output in base-64 output format\n"
"      -c               -- write output in canonical format\n"
"      -l               -- suppress linefeeds after output\n"
"   More than one output format can be requested at once.\n"
" There is normally a line-width of 75 on output, but:\n"
"      -w width         -- changes line width to specified width.\n"
"                          (0 implies no line-width constraint)\n"
" The default switches are: -p -a -b -c -x\n"
" Typical usage: cat certificate-file | sexp -a -x \n";

/*************************************************************************/
/* main(argc,argv)
 */
int main(int argc, char **argv) {
      char *c;
      bool  swa = true,
            swb = true,
            swc = true,
            swp = true,
            sws = false,
            swx = true,
            swl = false;
      int i;
      int ret = -1;
      sexp_exception::set_interactive(true);
      std::ifstream* ifs = nullptr;
      sexpInputStream* is = nullptr;
      std::ofstream* ofs = nullptr;
      sexpOutputStream *os = nullptr;
      try {
            sexpObject *object;

            is = new sexpInputStream(&std::cin);
            os = new sexpOutputStream(&std::cout);

            /* process switches */
            if (argc>1) swa = swb = swc = swp = sws = swx = swl = false;
            for (i=1; i<argc; i++) {
                  c = argv[i];
                  if (*c != '-') throw sexp_exception(std::string("Unrecognized switch ") + c, sexp_exception::error, EOF);
                  c++;
                  if (*c == 'a') swa = true; /* advanced output */
	            else if (*c == 'b') swb = true;/* base-64 output */
	            else if (*c == 'c') swc = true; /* canonical output */
                  else if (*c == 'h') { /* help */
                        std::cout << help;
                        std::cout.flush();
	                  exit(0);
      	      }
                  else if (*c == 'i') { /* input file */
	                  if (i+1<argc) i++;
                        ifs = new std::ifstream(argv[i], std::ifstream::binary);
	                  if (ifs->fail()) ErrorMessage(sexp_exception::error,"Can't open input file.",0,0, EOF);
                        is->setInput(ifs);
	            }
                  else if (*c == 'l') swl = true; /* suppress linefeeds after output */
                  else if (*c == 'o') { /* output file */
	                  if (i+1<argc) i++;
                        ofs = new std::ofstream(argv[i], std::ifstream::binary);
	                  if (ofs->fail()) ErrorMessage(sexp_exception::error,"Can't open output file.",0,0, EOF);
                        os->setOutput(ofs);
	            }
                  else if (*c == 'p') swp = true; /* prompt for input */
                  else if (*c == 's') sws = true; /* treat input as one big string */
                  else if (*c == 'w') { /* set output width */
	                  if (i+1<argc) i++;
	                  os->setMaxColumn(atoi(argv[i]));
	            }
                  else if (*c == 'x') swx = true; /* execute repeatedly */
                  else throw sexp_exception(std::string("Unrecognized switch ") + argv[i], sexp_exception::error, EOF);
            }

            if (swa == false && swb == false && swc == false)
            swc = true;  /* must have some output format! */

            /* main loop */
            if (swp == 0) is->getChar();
            else is->nextChar = -2;  /* this is not EOF */
            while (is->nextChar != EOF) {
                  if (swp) {
                        std::cout << "Input:" << std::endl;
                        std::cout.flush();
                  }

                  is->setByteSize(8);
                  if (is->nextChar == -2) is->getChar();

                  is->skipWhiteSpace();
                  if (is->nextChar == EOF) break;

                  if (sws == false) object = is->scanObject();
                  else              object = is->scanToEOF();

                  if (swc) {
                        if (swp) {
                              std::cout << "Canonical output:" << std::endl;
                              std::cout.flush();
	                        os->newLine(sexpOutputStream::advanced);
	                  }
	                  object->printCanonical(os);
	                  if (!swl) {
                              std::cout << std::endl;
                              std::cout.flush();
                        }
	            }

                  if (swb) {
                        if (swp) {
                              std::cout << "Base64 (of canonical) output:" << std::endl;
                              std::cout.flush();
	                        os->newLine(sexpOutputStream::advanced);
	                  }
      	            os->printBase64(object);
	                  if (!swl) {
                              std::cout << std::endl;
                              std::cout.flush();
                        }
	            }

                  if (swa) {
                        if (swp) {
                              std::cout << "Advanced transport output:" << std::endl;
                              std::cout.flush();
	                        os->newLine(sexpOutputStream::advanced);
	                  }
	                  os->printAdvanced(object);
	                  if (!swl) {
                              std::cout << std::endl;
                              std::cout.flush();
                        }
	            }

                  if (!swx) break;
                  if (!swp) is->skipWhiteSpace();
                  else if (!swl) {
                        std::cout << std::endl;
                        std::cout.flush();
                  }
            }
            ret = 0;
      }
      catch(sexp_exception e) {
            std::cout << e.what() << std::endl;
      }
      catch(...) {
            std::cout << "UNEXPECTED ERROR" << std::endl;
      }
      if (is) delete is;
      if (ifs) delete ifs;
      if (os) delete os;
      if (ofs) delete ofs;
      return ret;
}