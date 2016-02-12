libvmod-i18n
============

[![Build Status](https://travis-ci.org/cosimo/libvmod-i18n.svg?branch=master)](https://travis-ci.org/cosimo/libvmod-i18n)

Normalize and filter all the incoming requests' Accept-Language headers and
reduce them to just the languages your website or service supports.

This vmod is the newer, shinier version of my [varnish-accept-language VCL
module](https://github.com/cosimo/varnish-accept-language), which should be
considered deprecated.


What is this again?
-------------------

This is a software module (vmod) you use together with the Varnish HTTP reverse
proxy, when your website is localized in different languages, and you want to
be able to cache objects according to the language.

**WARNING**

This module consists of C code. Your Varnish might explode. YMMV.
Don't use it in production if you don't know what you're doing.
We and many other people are using it in production, but we _don't_ know what
we're doing :).


Why would you want this?
------------------------

Let's say your website supports English and Japanese languages.

Your client browsers will send every possible Accept-Language header on Earth.
If you enable "Vary: Accept-Language" on Varnish or on your backends, the cache
hit ratio will rapidly drop, because of the huge variations in
Accept-Language string contents.

With this vmod, you can rewrite the Accept-Language header to just "en" or
"ja", depending on your client settings. If no good match occurs, for example
if your clients are asking for a language that is not English nor Japanese,
you can still select a default language for your website.

If you choose to to rewrite the original Accept-Language header,
this language normalization will be completely transparent to your backend.


Module interface
----------------


== `i18n.match(STRING)`

Reads the Accept-Language header (or any other string passed to it), and
carries out Accept-Language parsing. The (language, q) pairs found are
sorted by priority (q), and the first language that is also declared as
supported (through the `i18n.supported_languages()` call, gets returned.

If no languages are supported, all languages are assumed to be supported.

If no default language has been specified, English is assumed ("en").

Example:

    sub vcl_recv {
        set req.http.Lang = i18n.match(req.http.Accept-Language);
    }

== `i18n.default_language(STRING)`

Typically used in `vcl_init()`, to specify which language should be assumed
as default when Accept-Language is either empty, or none of the languages in
it are supported.

If no default language is specified, English is assumed ("en").

== `is_supported(STRING)`

Returns 0 if the specified language is supported, otherwise 1.

== `supported_languages(STRING)`

Typically used in `vcl_init()`, to specify which languages your site or service
supports. Note that this is a string, not a list, so you need to specify the
list of supported languages as a colon-separated.

Example:

    sub vcl_init {
        i18n.supported_languages("de:fr:fr-CA:en:hi:it:ja");
    }

== `parse(STRING, STRING)`

Internal function that performs the Accept-Language string parsing (first
parameter). The second parameter is the default language to be returned if
no language is supported or Accept-Language is empty.

If you're not sure about using this function, it's because you shouldn't be
using it. Use `match()` instead.
