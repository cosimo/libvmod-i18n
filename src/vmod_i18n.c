/**
 * This code is offered under the Open Source BSD license.
 * 
 * Copyright (c) 2016 Opera Software. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 *  * Neither the name of Opera Software nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * DISCLAIMER OF WARRANTY
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>

#include "vrt.h"
#include "bin/varnishd/cache.h"

#include "vcc_if.h"

#define DEFAULT_FALLBACK_LANGUAGE "en"
#define HDR_MAXLEN 256
#define LANG_LIST_SIZE 8
#define LANG_MAXLEN 12

struct language {
    unsigned magic;
#define LANG_MAGIC      0x53537101
    char name[LANG_MAXLEN];
    float q;
};

struct vmod_i18n {
    unsigned        magic;
#define VMOD_I18N_MAGIC  0x27032703
    char           *default_language;
    char           *supported_languages;
};

#define PUSH_LANG(x,y) { \
    /* fprintf(stderr, "Pushing lang [%d] %s %.4f\n", curr_lang, x, y); */ \
    /* We have to copy, otherwise root_lang will be the same every time */ \
    strncpy(pl[curr_lang].name, x, LANG_MAXLEN); \
    pl[curr_lang].q = y;       \
    curr_lang++;               \
}

/* Unloading of the vmod private data structure */
static void
vmod_free(void *priv)
{
    struct vmod_i18n *v;

    if (priv) {
        CAST_OBJ_NOTNULL(v, priv, VMOD_I18N_MAGIC);

        if (v->default_language)
            free(v->default_language);

        if (v->supported_languages)
            free(v->supported_languages);

        FREE_OBJ(v);
    }
}


static struct vmod_i18n *
vmod_get(struct vmod_priv *priv)
{
    struct vmod_i18n *v;

    AN(priv);

    if (priv->priv) {
        CAST_OBJ_NOTNULL(v, priv->priv, VMOD_I18N_MAGIC);
    }
    else {
        ALLOC_OBJ(v, VMOD_I18N_MAGIC);
        AN(v);
        priv->free = vmod_free;
        priv->priv = v;
    }

    return (v);
}


int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf) {
    struct vmod_i18n *v;
    v = vmod_get(priv);
    return (0);
}


void
vmod_default_language(struct sess *sp, struct vmod_priv *priv,
    const char *def_lang)
{
    struct vmod_i18n *v;

    (void)sp;
    AN(priv);

    v = vmod_get(priv);

    /* Deallocate the previous string (if any).
       For when i18n.default_language() is used outside of vcl_init().
       This may happen if you're serving two domains with the same VCL. */
    if (v->default_language) {
        free(v->default_language);
        v->default_language = NULL;
    }

    v->default_language = strdup(def_lang);
    VSL(SLT_VCL_Log, 0, "i18n-vmod: default language set to '%s'", v->default_language);
}


void
vmod_supported_languages(struct sess *sp, struct vmod_priv *priv,
    const char *lang_list)
{
    struct vmod_i18n *v;
    char *p;
    unsigned u;

    (void)sp;
    AN(priv);

    v = vmod_get(priv);

    /* Deallocate the pre-existing string (if any)
       For when i18n.supported_languages() is used outside of vcl_init().
       This may happen if you're serving two domains with the same VCL. */
    if (v->supported_languages) {
        //VSL(SLT_VCL_Log, 0, "i18n-vmod: freeing and realloc of supported_languages");
        free(v->supported_languages);
        v->supported_languages = NULL;
    }

    p = malloc(strlen(lang_list) + 3);
    if (p == NULL) {
        VSL(SLT_VCL_Log, 0, "i18n-vmod: no storage for supported_languages");
        return;
    }

    strcpy(p, ":");
    strcat(p, lang_list);
    strcat(p, ":");

    v->supported_languages = p;
    VSL(SLT_VCL_Log, 0, "i18n-vmod: supported languages set to '%s'", v->supported_languages);
}


/* Checks if a given language is in the static list of the ones we support.

   Note: if you never call `i18n.supported_languages()` in your vcl_init
   to declare which are your supported languages, the code will assume that
   *any* language is supported. This is to make it work even with no
   configuration.
*/
int
vmod_is_supported(struct sess *sp, struct vmod_priv *priv, const char *lang)
{
    struct vmod_i18n *v;
    const char *supported_languages;
    char match_str[LANG_MAXLEN + 3] = "";  /* :, :, \0 = 3 */
    int is_supported = 0;

    (void)sp;
    AN(priv);
    v = vmod_get(priv);
    AN(v);

    supported_languages = v->supported_languages;

    if (supported_languages == NULL) {
        //VSL(SLT_VCL_Log, 0, "no supported_languages configuration");
        is_supported = 1;
    }
    else {
        /* Search ":<lang>:" in supported languages string */
        strncpy(match_str, ":", 1);
        strncat(match_str, lang, LANG_MAXLEN);
        strncat(match_str, ":\0", 2);

        /* We want to match 'zh-cn' and 'zh-CN' too */
        if (strcasestr(supported_languages, match_str))
            is_supported = 1;
    }

    return is_supported;
}

/* Used by qsort() below */
int sort_by_q(const void *x, const void *y) {
    struct language *a = (struct language *)x;
    struct language *b = (struct language *)y;
    if (a->q > b->q) return -1;
    if (a->q < b->q) return 1;
    return 0;
}

/* Reads Accept-Language header (or any other string passed to it), and carries
   out Accept-Language parsing. The (language, q) pairs found are sorted by
   priority (q), and the first language that is also declared as supported
   (through the `supported_languages()' call, gets returned.

   If no languages are supported, all languages are assumed to be supported.

   If no default language has been specified, English is assumed ("en").
*/
const char *
vmod_match(struct sess *sp, struct vmod_priv *priv, const char *accept_lang)
{
    struct vmod_i18n *v;
    char *lang_tok, *def, *header, *pos, *q_spec, *pref_name, *p;
    char root_lang[3];
    char header_copy[HDR_MAXLEN];
    unsigned curr_lang = 0, u, l;
    float q;
    struct language pl[LANG_LIST_SIZE], *pref_lang;

    AN(priv);
    v = vmod_get(priv);
    CHECK_OBJ_NOTNULL(v, VMOD_I18N_MAGIC);

    //VSL(SLT_VCL_Log, 0, "i18n-vmod: parsing '%s'", accept_lang);

    def = v->default_language;

    /* Empty Accept-Language header */
    if (accept_lang == NULL || strlen(accept_lang) == 0) {
        goto return_language;
    }

    /* Too long Accept-Language header. Let's cowardly refuse to parse it. */
    if (strlen(accept_lang) + 1 >= HDR_MAXLEN) {
        VSL(SLT_VCL_Log, 0, "i18n-vmod: Accept-Language string too long");
        goto return_language;
    }

    /* Empty or default string, return immediately.
       An empty language list (priv->pl) will trigger the return of the
       default language. */
    if ((def != NULL) && (0 == strcasecmp(accept_lang, def))) {
        goto return_language;
    }

    /*
     * Tokenize Accept-Language.
     */
    header = strncpy(header_copy, accept_lang, sizeof(header_copy));

    while ((lang_tok = strtok_r(header, " ,", &pos))) {

        q = 1.0;

        if ((q_spec = strstr(lang_tok, ";q="))) {
            /* Truncate language name before ';' */
            *q_spec = '\0';
            /* Get q value */
            sscanf(q_spec + 3, "%f", &q);
        }

        /* Wildcard language '*' should be last in list */
        if ((*lang_tok) == '*')
            q = 0.0;

        /* Push in the prioritized list */
        if (vmod_is_supported(sp, priv, lang_tok)) {
            //VSL(SLT_VCL_Log, 0, "i18n-vmod: PUSH_LANG %s %.3f", lang_tok, q);
            PUSH_LANG(lang_tok, q);
        }

        /* For cases like 'en-GB', we also want the root language in the final list */
        if ('-' == lang_tok[2]) {
            root_lang[0] = lang_tok[0];
            root_lang[1] = lang_tok[1];
            root_lang[2] = '\0';
            //VSL(SLT_VCL_Log, 0, "i18n-vmod: added root language '%s'", root_lang);
            if (vmod_is_supported(sp, priv, root_lang)) {
                PUSH_LANG(root_lang, q - 0.001);
            }
        }

        /* For strtok_r() to proceed from where it left off */
        header = NULL;

        /* Break out if stored max no. of languages */
        if (curr_lang >= LANG_LIST_SIZE)
            break;
    }

    //VSL(SLT_VCL_Log, 0, "i18n-vmod: parsed %i languages", i);

    return_language:

    if (curr_lang == 0) {
        pref_lang = NULL;
    }
    else {
        /* Sort the languages in descending priority order */
        qsort(pl, curr_lang, sizeof(struct language), &sort_by_q);
        /* First language in the resulting list is the one we pick */
        pref_lang = &pl[0];
    }

    /* No language parsed, output the set default */
    if (pref_lang == NULL) {
        //VSL(SLT_VCL_Log, 0, "i18n-vmod: return default language '%s'", v->default_language);
        pref_name = v->default_language;
    }
    else {
        pref_name = pref_lang->name;
    }

    u = WS_Reserve(sp->ws, 0);
    p = sp->ws->f;
    l = snprintf(p, u, "%s", pref_name != NULL
        ? pref_name
        : DEFAULT_FALLBACK_LANGUAGE);
    l++;

    if (l > u) {
        /* No workspace memory, reset and leave */
        WS_Release(sp->ws, 0);
        return (NULL);
    }

    WS_Release(sp->ws, l);
    return p;
}
