//  crm_preprocessor.c  - Controllable Regex Mutilator,  version v1.0
//  Copyright 2001-2007  William S. Yerazunis, all rights reserved.
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
//  Other licenses may be negotiated; contact the
//  author for details.
//
//  include some standard files
#include "crm114_sysincludes.h"

//  include any local crm114 configuration file
#include "crm114_config.h"

//  include the crm114 data structures file
#include "crm114_structs.h"

//  and include the routine declarations file
#include "crm114.h"



//
//       the actual textual representations of the flags, with their values
//     DON'T FORGET TO ALSO MODIFY THIS IN crm114_structs.h !!


const FLAG_DEF crm_flags[] =
{
  { "fromstart", CRM_FROMSTART },    /* bit 0 */
  { "fromnext", CRM_FROMNEXT },
  { "fromend", CRM_FROMEND },
  { "newend", CRM_NEWEND },
  { "fromcurrent", CRM_FROMCURRENT },
  { "nocase", CRM_NOCASE },
  { "absent", CRM_ABSENT },
  { "basic", CRM_BASIC },
  { "backwards", CRM_BACKWARDS },
  { "literal", CRM_LITERAL },
  { "nomultiline", CRM_BYLINE },    /* bit 10 */
  { "byline", CRM_BYLINE },         /* bit 10 */
  { "bychar", CRM_BYCHAR },
  { "string", CRM_BYCHAR },
  { "bychunk", CRM_BYCHUNK },
  { "byeof", CRM_BYEOF },
  { "eofaccepts", CRM_EOFACCEPTS },
  { "eofretry", CRM_EOFRETRY },
  { "append", CRM_APPEND },
  { "keep", CRM_KEEP },
  { "async", CRM_ASYNC },
  { "refute", CRM_REFUTE },
  { "microgroom", CRM_MICROGROOM },    /* bit 20 */
  { "markovian", CRM_MARKOVIAN },
  { "markov", CRM_MARKOVIAN },
  { "osb", CRM_OSB_BAYES },
  { "correlate", CRM_CORRELATE },
  { "winnow", CRM_OSB_WINNOW },
  { "chi2", CRM_CHI2 },           /* bit 25 */
  { "unique", CRM_UNIQUE },       /* bit 26 */
  { "entropy", CRM_ENTROPY },     /* bit 27 */
  { "entropic", CRM_ENTROPY },    /* bit 27 */
  { "osbf", CRM_OSBF },
  { "hyperspace", CRM_HYPERSPACE },
  { "unigram", CRM_UNIGRAM },    /* bit 30 */
  { "crosslink", CRM_CROSSLINK },
  { "lineedit", CRM_READLINE },    /* bit 32! */
  { "default", CRM_DEFAULT },      /* bit 33! */
  { "sks", CRM_SKS },
  { "svm", CRM_SVM },
  { "fscm", CRM_FSCM },
  { "scm", CRM_SCM },
  { "neural", CRM_NEURAL_NET },
  { "flat", CRM_FLAT },
  { NULL, 0 }     /* [i_a] sentinel */
};

/* #define CRM_MAXFLAGS 42   [i_a] unused in the new code */




//    The magic flag parser.  Given a string of input, return the
//    codes that were found as the (long int) return value.  If an
//    unrecognized code is found, squalk an error (whether it is fatal
//    or not is another issue)
//
//    Note that since flags (like variables) are always ASCII, we don't
//    need to worry about 8-bit-safety.
//
uint64_t crm_flagparse(char *input, long inlen)  //  the user input
{
  char flagtext[MAX_PATTERN];
  char *remtext;
  long remlen;
  char *wtext;
  long flagsearch_start_here;
  long wstart;
  long wlen;
  uint64_t outcode;

  int done;
  int i;
  int j;
  int k;
  int recog_flag;

  outcode = 0;

  if (inlen >= MAX_PATTERN) inlen = MAX_PATTERN - 1;
  memmove(flagtext, input, inlen);
  flagtext[inlen] = 0;

  if (internal_trace)
    fprintf(stderr, "Flag string: %s\n", flagtext);

  //  now loop on thru the nextwords,
  remtext = flagtext;
  done = 0;
  remlen = inlen;
  wstart = 0;
  wlen = 0;
  flagsearch_start_here = 0;
  while (!done && remlen > 0)
  {
    i = crm_nextword(remtext, remlen, flagsearch_start_here, &wstart, &wlen);
    flagsearch_start_here = wstart + wlen + 1;
    if (wlen > 0)
    {
      //    We got a word, so aim wtext at it
      wtext = &(remtext[wstart]);
      if (internal_trace)
      {
        fprintf(stderr, "found flag, len %ld: %.*s\n",
                wlen, (int)wlen, wtext);
      }

      //    find sch in our table, squalk a nonfatal/fatal if necessary.
      recog_flag = 0;
      for (j = 0; crm_flags[j].string != NULL; j++)      /* [i_a] loop until we've hit the sentinel at the end of the table */
      {
        // fprintf(stderr, " Trying %s (%ld) at pos = %d\n", crm_flags[j].string, crm_flags[j].value, j );

        // make sure the flags are ordered properly; must match with crm114_structs.h defs, but that's kinda hard to check
        CRM_ASSERT(crm_flags[j].value > 0);
        CRM_ASSERT(j > 0 ? crm_flags[j].value >= crm_flags[j - 1].value : 1);
        CRM_ASSERT(j > 0 ? crm_flags[j].value == crm_flags[j - 1].value ? 1 : crm_flags[j].value ==
                   (crm_flags[j - 1].value << 1LL) : 1);
        CRM_ASSERT(j == 0 ? crm_flags[j].value == 1 : 1);

        k = strlen(crm_flags[j].string);
        if (k == wlen
            && 0 == strncasecmp(wtext, crm_flags[j].string, k))
        {
          //    mark this flag as valid so we don't squalk an error
          recog_flag = 1;
          //     and OR this into our outcode
          outcode |= crm_flags[j].value;
          if (user_trace)
          {
            fprintf(stderr, "Mode #%d, '%s' turned on. \n",
                    j,
                    crm_flags[j].string);
          }
        }
      }

      //   check to see if we need to squalk an error condition
      if (recog_flag == 0)
      {
        long q;
        char foo[129];
        strncpy(foo, wtext, wlen < 128 ? wlen : 128);
        foo[wlen < 128 ? wlen : 128] = 0;
        q = nonfatalerror("Darn...  unrecognized flag :", foo);
      }

      //  and finally,  move sch up to point at the remaining string
      if (remlen <= 0) done = 1;
    }
    else
    {
      done = 1;
    }
  }

  if (internal_trace)
    fprintf(stderr, "Flag code is : %llx\n", (unsigned long long)outcode);

  return outcode;
}

//     Get the next word in a string.  "word" is defined by the
//     continuous span of characters that are above ascii ! (> hex 0x20
//
//     The search starts at the "start" position given; the start position
//     is updated on each call and so is mutilated.  To step through a
//     arglist, you must add the returned value of "len" to the returned
//     value of start!
//
//     The returned value is 0/1 as to whether we found
//     a valid word, and *start and *length, which give it's position.
//
long crm_nextword(const char *input,
                  long        inlen,
                  long        starthere,
                  long       *start,
                  long       *len)
{
  *start = starthere;
  *len = 0;
  //   find start of string (if it exists)
  while (*start < inlen && input[*start] <= 0x20) *start = *start + 1;

  //  check - did we hit the end and still be invalid?  If so, return 0
  if (*start == inlen) return 0;

  //    if we get to here, then we have a valid string.
  *len = 0;
  while ((*start + *len) < inlen
         && input[*start + *len] > 0x20) *len = *len + 1;

  return (*len) > 0;
}



//
//    experimental code for a statement-type-sensitive parser.
//   Not in use yet... but someday... goal is to provide better error
//   detection.

int crm_profiled_statement_parse(char *in,
                                 long slen,
                                 ARGPARSE_BLOCK *apb,
                                 long amin, long amax,
                                 long pmin, long pmax,
                                 long bmin, long bmax,
                                 long smin, long smax)
{
  return 0;
}

//      parse a CRM114 statement; this is mostly a setup routine for
//     the generic parser.

int crm_statement_parse(char           *in,
                        long            slen,
                        ARGPARSE_BLOCK *apb)
{
#define CRM_STATEMENT_PARSE_MAXARG 10
  int i,  k;

  int ftype[CRM_STATEMENT_PARSE_MAXARG];
  int fstart[CRM_STATEMENT_PARSE_MAXARG];
  int flen[CRM_STATEMENT_PARSE_MAXARG];

  //     we call the generic parser with the right args to slice and
  //     dice the incoming statement into declension-delimited parts
  k = crm_generic_parse_line(in,
                             slen,
                             CRM_STATEMENT_PARSE_MAXARG,
                             ftype,
                             fstart,
                             flen);

  //   now we have all these nice chunks... we split them up into the
  //   various allowed categories.


  //   start out with empties on each possible chunk
  apb->a1start = NULL;
  apb->a1len = 0;
  apb->p1start = NULL;
  apb->p1len = 0;
  apb->p2start = NULL;
  apb->p2len = 0;
  apb->p3start = NULL;
  apb->p3len = 0;
  apb->b1start = NULL;
  apb->b1len = 0;
  apb->s1start = NULL;
  apb->s1len = 0;
  apb->s2start = NULL;
  apb->s2len = 0;

  //   Scan through the incoming chunks
  for (i = 0; i < k; i++)
  {
    switch (ftype[i])
    {
    case CRM_ANGLES:
      {
        //  Grab the angles, if we don't have one already
        if (apb->a1start == NULL)
        {
          apb->a1start = &in[fstart[i]];
          apb->a1len = flen[i];
        }
        else
        {
          nonfatalerror(
            "There are multiple flag sets on this line.",
            " ignoring all but the first");
        }
      }
      break;

    case CRM_PARENS:
      {
        //  grab a set of parens, cascading till we find an one
        if (apb->p1start == NULL)
        {
          apb->p1start = &in[fstart[i]];
          apb->p1len = flen[i];
        }
        else if (apb->p2start == NULL)
        {
          apb->p2start = &in[fstart[i]];
          apb->p2len = flen[i];
        }
        else if (apb->p3start == NULL)
        {
          apb->p3start = &in[fstart[i]];
          apb->p3len = flen[i];
        }
        else
        {
          nonfatalerror(
            "Too many parenthesized varlists.",
            "ignoring the excess varlists.");
        }
      }
      break;

    case CRM_BOXES:
      {
        //  Grab the angles, if we don't have one already
        if (apb->b1start == NULL)
        {
          apb->b1start = &in[fstart[i]];
          apb->b1len = flen[i];
        }
        else
        {
          nonfatalerror(
            "There are multiple domain limits on this line.",
            " ignoring all but the first");
        }
      }
      break;

    case CRM_SLASHES:
      {
        //  grab a set of parens, cascading till we find an one
        if (apb->s1start == NULL)
        {
          apb->s1start = &in[fstart[i]];
          apb->s1len = flen[i];
        }
        else if (apb->s2start == NULL)
        {
          apb->s2start = &in[fstart[i]];
          apb->s2len = flen[i];
        }
        else
        {
          nonfatalerror(
            "There are too many regex sets in this statement,",
            " ignoring all but the first.");
        }
      }
      break;

    default:
      fatalerror_ex(SRC_LOC(),
                    "Declensional parser returned an undefined typecode %d@%d/%d! "
                    "What the HECK did you do to cause this?",
                    ftype[i], i, k);
      break;
    }
  }
  return k;      // return value is how many declensional arguments we found.
}


//     The new and improved line core parser routine.  Instead of
//     being totally ad hoc, this new parser actually retains context
//     durng the parse.
//
//     this hopefully will keep the parser from getting confused by [] in
//     the slash matching and other such abominations.
//
//     (one way to view this style of parsing is that each arg in a
//     CRM114 statement is "declined" by it's delimiters to determine
//     what role this variable is to play in the statement.  Kinda like
//     Latin - to a major extent, you can mix the parts around and it
//     won't make any difference.
//
// WARNING: any of the returned items my have any length (even larger
//          than MAX_PATTERN), so make sure you (the caller) can cope with this.
//
int crm_generic_parse_line(
  char *txt,                          //   the start of the program line
  int   len,                          //   how long is the line
  int   maxargs,                      //   howm many things to search for (max)
  int  *ftype,                        //   type of thing found (index by schars)
  int  *fstart,                       //   starting location of found arg
  int  *flen)                         //   length of found arg
{
  //    the general algorithm here is to move along the input line,
  //    looking for one of the characters in schars.  When we find it,
  //    we lock onto that and commit to finding an arg of that type.
  //    We then start scanning ahead keeping count of schars minus echars.
  //    when the count hits zero, it's end for that arg and we move onward
  //    to the next arg, with the same procedure.
  //
  //    note that when we are scanning for a new arg, we are open to args
  //    of any type (as defined by the members of schars, while in an arg
  //    we are looking only for the unescaped outstanding echar and are blind
  //    to everything else.
  //
  //    when not in an arg, we do not have any escape character active.
  //
  //     We return the number of args found

  int chidx;
  int argc;
  int i;
  int itype;
  int dstpos;
  char finchar = 0;

  int submode = 0;

  //    zeroize the outputs to start...
  for (i = 0; i < maxargs; i++)
  {
    ftype[i] = CRM_FIND_ACTION;
    fstart[i] = 0;
    flen[i] = 0;
  }

  //    scan forward, looking for any member of schars
  argc = 0;
  itype = CRM_FIND_ACTION;

  if (internal_trace)
  {
    fprintf(stderr, " declensional parsing for %d chars on: %.*s%s\n",
            len,
            (len > 1024 ? 1024 : len),
            txt,
            (len > 1024 ? "(truncated...)" : ""));
  }

  for (dstpos = chidx = 0; chidx < len && argc < maxargs; chidx++)
  {
    char curchar = txt[chidx];

    switch (itype)
    {
    default:
      CRM_ASSERT_EX(0, "Should never get here while parsing a statement");

    case CRM_FIND_ACTION:
      // we need to decode the command (ACTION) itself:
      // allowed are:
      //
      //  {
      //  }
      //  :label:
      //  action
      //
      if (crm_isspace(curchar))
        continue;
      switch (curchar)
      {
      case '{':
      case '}':
        // no args allowed: check that!
        for (chidx++; chidx < len; chidx++)
        {
          if (!crm_isspace(txt[chidx]))
          {
            nonfatalerror_ex(SRC_LOC(
                             ),
                             " Curly braces delineate code sections. Is a action/command missing here?\n Bug in statement?\n --> %.*s%s",
                             (len > 1024 ? 1024 : len),
                             txt,
                             (len > 1024 ? "(...truncated)" : ""));
            return 0;
          }
        }
        return 0;

      case ':':
        // scan the label; we're lazy so we just track down the
        // terminating ':' there.
        itype = CRM_PARSE_LABEL;
        submode = 1;
        break;

      default:
        if (crm_isalpha(curchar))
        {
          itype = CRM_PARSE_ACTION;
        }
        else
        {
          nonfatalerror_ex(SRC_LOC(),
                           " Unidentified action/command '%c'(HEX:%02X) in statement.\n Bug in statement?\n --> %.*s%s",
                           (crm_isprint(curchar) ? curchar : '.'),
                           (int)curchar,
                           (len > 1024 ? 1024 : len),
                           txt,
                           (len > 1024 ? "(...truncated)" : ""));
          return 0;
        }
        break;
      }
      break;

    case CRM_PARSE_LABEL:
      switch (curchar)
      {
      case ':':
        submode++;
        if (submode == 2)
        {
          // counted both start and end ':' --> label has now been scanned

          submode = 0;                  // reset submode
          itype = CRM_FIND_ARG_SECTION; // start to parse arg sections now.
        }
        break;

      default:
        break;
      }
      break;

    case CRM_PARSE_ACTION:
      // parsing the command; when it ends, it may be followed by any of the <>[]()// sections
      switch (curchar)
      {
      case '<':
      case '[':
      case '(':
      case '/':
        itype = CRM_FIND_ARG_SECTION;
        break; // get to case CRM_FIND_ARG_SECTION below via the fallthrough there

      default:
        if (crm_isspace(curchar))
        {
          // end of action found
          itype = CRM_FIND_ARG_SECTION;
        }
        else if (!crm_isalpha(curchar))
        {
          nonfatalerror_ex(SRC_LOC(),
                           " Unidentified action/command '%.*s' in statement.\n Bug in statement?\n --> %.*s%s",
                           (chidx + 1),
                           txt,
                           (len > 1024 ? 1024 : len),
                           txt,
                           (len > 1024 ? "(...truncated)" : ""));
          return 0;
        }
        continue;
      }

      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // fallthrough! fallthrough! fallthrough! fallthrough! fallthrough!
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    case CRM_FIND_ARG_SECTION:
      //    is curchar one of the start chars?  (this is 8-bit-safe,
      //     because schars is always normal ASCII)
      submode = 0;
      switch (curchar)
      {
      case '<':
        itype = CRM_ANGLES;
        finchar = '>';
        break;

      case '(':
        itype = CRM_PARENS;
        finchar = ')';
        break;

      case '[':
        itype = CRM_BOXES;
        finchar = ']';
        submode = 1;         // check for special case: "[:var: /regex/]"
        break;

      case '/':
        itype = CRM_SLASHES;
        finchar = '/';
        break;

      default:
        if (!crm_isspace(curchar))
        {
          nonfatalerror_ex(SRC_LOC(),
                           " The statement contains an unidentified operand delimiter '%c'(HEX:%02X). "
                           "Only these are currently supported: <>()[]//\n Bug in statement?\n --> %.*s%s",
                           (crm_isprint(curchar) ? curchar : '.'),
                           (int)curchar,
                           (len > 1024 ? 1024 : len),
                           txt,
                           (len > 1024 ? "(...truncated)" : ""));
          return argc;
        }
        continue;
      }

      // section delimiter detected; mark section
      ftype[argc] = itype;
      dstpos = fstart[argc] = chidx + 1;
      break;

    case CRM_ANGLES:
    case CRM_PARENS:
    case CRM_BOXES:
    case CRM_SLASHES:
      // nope, we're in an arg, so we check for unescaped schar
      // and fchar characters

      // but first, check for special cases:
      switch (submode)
      {
      default:
      case 0:
        break;

      case 1:
        // chars must be whitespace or var marker ':' to make
        // us go on looking for this case...
        if (crm_isspace(curchar))
          break;
        if (curchar == ':')
          submode = 2;
        else
          submode = 0; // it's not going to happen here...
        break;

      case 2:
        // chars must be a var-name, terminated with a ':' marker.
        // We're lazy, so check for the ':' only...
        if (curchar == ':')
          submode = 3;
        break;

      case 3:
        // chars must be whitespace or regex start marker '/'
        if (crm_isspace(curchar))
          break;
        if (curchar == '/')
        {
          // WHEN we've HIT the '/', the character which
          // requires MANDATORY escaping becomes '/', but only until
          // we hit the terminating '/' of the regex.
          //
          // The solution we choose here is to scan ahead to the terminating
          // '/' right here, thus duplicating the unescape logic below.
          // This, however, prevents the main flow from getting cluttered with
          // our 'special case' handling here.
          txt[dstpos++] = txt[chidx++];
          for ( ; chidx < len; chidx++)
          {
            curchar = txt[chidx];
            if (curchar == '\\')
            {
              // escape: is it escaping our regex terminator, or is it escaping another?
              // if another, copy all as-is:
              if (txt[chidx + 1] != '/')
              {
                // copy both as-is and skip
                txt[dstpos++] = txt[chidx++];
                txt[dstpos++] = txt[chidx];
              }
              else
              {
                // hit the '/' terminator in ESCAPED form: only copy the UNescaped terminator character
                // and continue.
                txt[dstpos++] = txt[++chidx];
              }
            }
            else if (curchar == '/')
            {
              // HIT the regex terminator!
              // end 'special case' processing and continue in the main loop again.
              break;
            }
            else
            {
              // regular char: copy as-is
              txt[dstpos++] = curchar;
            }
          }
        }
        else
        {
          // apparently the :var: is followed by a non-regex argument here. Nothing special.
        }
        submode = 0;                 // handling done.
        break;
      }

      if (curchar == '\\')
      {
        // escape: is it escaping our terminating fchar, or is it escaping another?
        // if another, copy all as-is:
        if (txt[chidx + 1] != finchar)
        {
          // copy both as-is and skip
          txt[dstpos++] = txt[chidx++];
          txt[dstpos++] = txt[chidx];
        }
        else
        {
          // hit the terminator in ESCAPED form: only copy the UNescaped terminator character
          // and continue.
          txt[dstpos++] = txt[++chidx];
        }
      }
      else
      {
        // no escape character here. Did we hit the fchar terminator yet?
        if (curchar == finchar)
        {
          // yes, we did!
          //
          //   we've found the end of the text arg.  Close it off and
          //   note it into the output vectors
          flen[argc] = dstpos - fstart[argc];

          // pad txt[] with spaces (following the terminator) to fixup after the unescaping:
          // make it look all right again ... ;-)
          txt[dstpos++] = curchar;
          for ( ; dstpos <= chidx;)
          {
            txt[dstpos++] = ' ';
          }

          if (internal_trace)
          {
            fprintf(stderr, " close %c at %d/(unescaped:%d) -- %.*s%s -- len %d\n",
                    curchar, dstpos, chidx,
                    CRM_MIN(flen[argc], 1024),
                    &txt[fstart[argc]],
                    (flen[argc] > 1024 ? "(truncated...)" : ""),
                    flen[argc]);
          }

          itype = CRM_FIND_ARG_SECTION;
          argc++;
        }
        else
        {
          // a regular char: move as-is
          txt[dstpos++] = curchar;
        }
      }
      break;
    }
  }

  if (itype >= 0)
  {
    int operand_len;
    int statement_len;

    flen[argc] = dstpos - fstart[argc];
    operand_len = flen[argc];
    // CRM_ASSERT(chidx >= 0);
    CRM_ASSERT(dstpos >= 0);
    statement_len = dstpos;
    nonfatalerror_ex(SRC_LOC(),
                     " The operand '%.*s'%s doesn't seem to end.  Bug in statement?\n --> %.*s",
                     (operand_len > 1024 ? 1024 : operand_len),
                     &txt[fstart[argc]],
                     (operand_len > 1024 ? "(...truncated)" : ""),
                     statement_len,
                     txt);
    argc++;
  }
  return argc;
}



//    and to avoid all the mumbo-jumbo, an easy way to get a copy of
//    an arg found by the declensional parser.
void crm_get_pgm_arg(char *to, long tolen, char *from, long fromlen)
{
  long len;

  if (to == NULL)
    return;

  if (from == NULL)
  {
    to[0] = 0;
  }
  else
  {
    len = tolen - 1;
    if (len > fromlen) len = fromlen;
    memmove(to, from, len);
    to[len] = 0;
  }
}


