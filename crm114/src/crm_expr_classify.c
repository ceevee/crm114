//  crm_expr_classify.c  - Controllable Regex Mutilator,  version v1.0
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

//  OSBF declarations
#include "crm114_osbf.h"



//     Dispatch a LEARN statement
//
int crm_expr_learn(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        VHT_CELL **vht,
        CSL_CELL *tdw)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int saved_ssfl;
    uint64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to learn:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i < 0)
    {
        int curstmt;
        int fev;
        CRM_ASSERT(i == -1 || i == -2);
        fev = 0;
        curstmt = csl->cstmt;
        if (i == -1)
            fev = nonfatalerror(errstr, "");
        if (i == -2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "LEARN is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //  keep the original value of the ssfl, because many learners
    //  mangle it and then it won't work right for other classifiers
    saved_ssfl = sparse_spectrum_file_length;

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.

    classifier_flags = apb->sflags;

#if 0
    //     Joe thinks that this should be a table or a loop.
    classifier_flags = classifier_flags
                       & (CRM_OSB_BAYES | CRM_CORRELATE | CRM_OSB_WINNOW | CRM_OSBF
                          | CRM_HYPERSPACE | CRM_ENTROPY | CRM_SVM | CRM_SKS | CRM_FSCM
                          | CRM_NEURAL_NET);
#endif

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 0, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_fast_substring_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_learn(csl, apb, vht, tdw, txt, start, len);
    }
    else
    {
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_learn(csl, apb, vht, tdw, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 3, "L", (unsigned long long int)classifier_flags);

    sparse_spectrum_file_length = saved_ssfl;

    return retval;
}

//      Dispatch a CLASSIFY statement
//
int crm_expr_classify(CSL_CELL *csl, ARGPARSE_BLOCK *apb,
        VHT_CELL **vht,
        CSL_CELL *tdw
                     )
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i > 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CLASSIFY is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 4, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_fast_substring_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_classify(csl, apb, vht, tdw, txt, start, len);
    }
    else
    {
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_classify(csl, apb, vht, tdw, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 5, "L", (unsigned long long int)classifier_flags);

    return retval;
}





//      Dispatch a MERGE statement
//
int crm_expr_css_merge(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i > 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CSSMERGE is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 6, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_css_merge(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_css_merge(csl, apb, txt, start, len);
    }
    else
    {
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_css_merge(csl, apb, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 7, "L", (unsigned long long int)classifier_flags);

    return retval;
}







//      Dispatch a DIFF statement
//
int crm_expr_css_diff(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i > 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CSSDIFF is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 8, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_css_diff(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_css_diff(csl, apb, txt, start, len);
    }
    else
    {
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_css_diff(csl, apb, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 9, "L", (unsigned long long int)classifier_flags);

    return retval;
}





//      Dispatch a BACKUP statement
//
int crm_expr_css_backup(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i > 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CSSBACKUP is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 10, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_css_backup(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_css_backup(csl, apb, txt, start, len);
    }
    else
    {
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_css_backup(csl, apb, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 11, "L", (unsigned long long int)classifier_flags);

    return retval;
}




//      Dispatch a RESTORE statement
//
int crm_expr_css_restore(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i > 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CSSRESTORE is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 12, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_css_restore(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_css_restore(csl, apb, txt, start, len);
    }
    else
    {
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_css_restore(csl, apb, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 13, "L", (unsigned long long int)classifier_flags);

    return retval;
}





//      Dispatch a INFO statement
//
int crm_expr_css_info(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i > 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CSSINFO is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 14, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_css_info(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_css_info(csl, apb, txt, start, len);
    }
    else
    {
        //    Default with no classifier specified
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_css_info(csl, apb, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 15, "L", (unsigned long long int)classifier_flags);

    return retval;
}





//      Dispatch a ANALYZE statement
//
int crm_expr_css_analyze(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i > 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CSSANALYZE is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 16, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_css_analyze(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_css_analyze(csl, apb, txt, start, len);
    }
    else
    {
        //    Default with no classifier specified
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_css_analyze(csl, apb, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 17, "L", (unsigned long long int)classifier_flags);

    return retval;
}





//      Dispatch a CREATE statement
//
int crm_expr_css_create(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i > 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CSSCREATE is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 18, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_css_create(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_css_create(csl, apb, txt, start, len);
    }
    else
    {
        //    Default with no classifier specified
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_css_create(csl, apb, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 19, "L", (unsigned long long int)classifier_flags);

    return retval;
}






//      Dispatch a MIGRATE statement
//
int crm_expr_css_migrate(CSL_CELL *csl, ARGPARSE_BLOCK *apb)
{
    char box_text[MAX_PATTERN];
    char errstr[MAX_PATTERN];
    int i;
    char *txt;
    int start;
    int len;
    int retval;
    int64_t classifier_flags = 0;
    int boxtxtlen;

    //            get start/length of the text we're going to classify:
    //
    CRM_ASSERT(apb != NULL);
    boxtxtlen = crm_get_pgm_arg(box_text, MAX_PATTERN, apb->b1start, apb->b1len);

    //  Use crm_restrictvar to get start & length to look at.
    i = crm_restrictvar(box_text, boxtxtlen,
                        NULL,
                        &txt,
                        &start,
                        &len,
                        errstr,
                        WIDTHOF(errstr));

    if (i > 0)
    {
        int curstmt;
        int fev;
        fev = 0;
        curstmt = csl->cstmt;
        if (i == 1)
            fev = nonfatalerror(errstr, "");
        if (i == 2)
            fev = fatalerror(errstr, "");
        //
        //     did the FAULT handler change the next statement to execute?
        //     If so, continue from there, otherwise, we FAIL.
        if (curstmt == csl->cstmt)
        {
#if defined(TOLERATE_FAIL_AND_OTHER_CASCADES)
            csl->next_stmt_due_to_fail = csl->mct[csl->cstmt]->fail_index;
#else
            csl->cstmt = csl->mct[csl->cstmt]->fail_index - 1;
#endif
            if (internal_trace)
            {
                fprintf(stderr, "CSSMIGRATE is jumping to statement line: %d/%d\n", csl->mct[csl->cstmt]->fail_index, csl->nstmts);
            }
            CRM_ASSERT(csl->cstmt >= 0);
            CRM_ASSERT(csl->cstmt <= csl->nstmts);
            csl->aliusstk[csl->mct[csl->cstmt]->nest_level] = -1;
        }
        return fev;
    }

    //            get our flags... the only ones we're interested in here
    //            are the ones that specify _which_ algorithm to use.
    classifier_flags = apb->sflags;

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 20, "L", (unsigned long long int)classifier_flags);

    if (classifier_flags & CRM_OSB_BAYES)
    {
        retval = crm_expr_osb_bayes_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_CORRELATE)
    {
        retval = crm_expr_correlate_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSB_WINNOW)
    {
        retval = crm_expr_osb_winnow_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_OSBF)
    {
        retval = crm_expr_osbf_bayes_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_HYPERSPACE)
    {
        retval = crm_expr_osb_hyperspace_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ENTROPY)
    {
        retval = crm_expr_bit_entropy_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SVM)
    {
        retval = crm_expr_svm_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_SKS)
    {
        retval = crm_expr_sks_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_FSCM)
    {
        retval = crm_expr_fscm_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_NEURAL_NET)
    {
        retval = crm_neural_net_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_BAYES)
    {
        retval = crm_expr_alt_osb_bayes_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSB_WINNOW)
    {
        retval = crm_expr_alt_osb_winnow_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_OSBF)
    {
        retval = crm_expr_alt_osbf_bayes_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_HYPERSPACE)
    {
        retval = crm_expr_alt_osb_hyperspace_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_ALT_MARKOVIAN)
    {
        retval = crm_expr_alt_markov_css_migrate(csl, apb, txt, start, len);
    }
    else if (classifier_flags & CRM_MARKOVIAN)
    {
        retval = crm_expr_markov_css_migrate(csl, apb, txt, start, len);
    }
    else
    {
        //    Default with no classifier specified
        apb->sflags |= CRM_AUTODETECT;
        retval = crm_expr_markov_css_migrate(csl, apb, txt, start, len);
    }

    crm_analysis_mark(&analysis_cfg, MARK_CLASSIFIER, 21, "L", (unsigned long long int)classifier_flags);

    return retval;
}


