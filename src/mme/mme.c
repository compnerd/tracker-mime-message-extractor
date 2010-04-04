/* vim: set et fdm=syntax sts=4 sw=4 ts=4 : */
/**
 * Copyright © 2010 Saleem Abdulrasool <compnerd@compnerd.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation and/or
 *    other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 **/

#include <gmime/gmime.h>
#include <libtracker-extract/tracker-extract.h>


static GMimeParser *
_mime_parser_for_uri (const gchar   *uri)
{
    GFile *file;
    GMimeStream *stream;

    file = g_file_new_for_uri (uri);
    stream = g_mime_stream_gio_new (file);
    return g_mime_parser_new_with_stream (stream);
}


static void
extract_message_rfc822 (const gchar             *uri,
                        TrackerSparqlBuilder    *preupdate,
                        TrackerSparqlBuilder    *metadata)
{
    GMimeParser *parser;
    GMimeMessage *message;
    const gchar *subject, *message_id;

    g_type_init ();
    g_mime_init (GMIME_ENABLE_RFC2047_WORKAROUNDS);

    parser = _mime_parser_for_uri (uri);

    message = g_mime_parser_construct_message (parser);
    if (! message)
        goto out;

    tracker_sparql_builder_predicate (metadata, "a");
    tracker_sparql_builder_object (metadata, "nmo:Message");

    subject = g_mime_message_get_subject (message);
    if (subject && g_utf8_validate (subject, -1, NULL)) {
        tracker_sparql_builder_predicate (metadata, "nmo:messageSubject");
        tracker_sparql_builder_object_string (metadata, subject);
    }

    message_id = g_mime_message_get_message_id (message);
    if (message_id && g_utf8_validate (message_id, -1, NULL)) {
        tracker_sparql_builder_predicate (metadata, "nmo:messageId");
        tracker_sparql_builder_object_string (metadata, message_id);
    }

out:
    g_mime_shutdown ();
}


static TrackerExtractData data[] =
{
    { "message/rfc822", extract_message_rfc822 },
    { NULL, NULL },
};


TrackerExtractData *
tracker_extract_get_data (void)
{
    return data;
}

