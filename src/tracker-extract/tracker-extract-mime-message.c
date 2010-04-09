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
#include <gmime/internet-address.h>

#include <libtracker-client/tracker-client.h>
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
_mark_addresses_as (TrackerSparqlBuilder    *preupdate,
                    TrackerSparqlBuilder    *metadata,
                    InternetAddressList     *ial,
                    const gchar * const      predicate)
{
    unsigned int i;

    for (i = 0; i < internet_address_list_length (ial); i++) {
        InternetAddress *ia = internet_address_list_get_address (ial, i);
        const gchar *fullname = NULL, *email_address = NULL;
        gchar *email_uri = NULL;

        fullname = internet_address_get_name (ia);

        if (INTERNET_ADDRESS_IS_MAILBOX (ia)) {
            email_address = internet_address_mailbox_get_addr (INTERNET_ADDRESS_MAILBOX (ia));
        }

        if (email_address) {
            email_uri = tracker_uri_printf_escaped ("mailto:%s", email_address);

            tracker_sparql_builder_insert_open (preupdate, NULL);
                tracker_sparql_builder_subject_iri (preupdate, email_uri);
                tracker_sparql_builder_predicate (preupdate, "rdf:type");
                tracker_sparql_builder_object (preupdate, "nco:EmailAddress");

                tracker_sparql_builder_subject_iri (preupdate, email_uri);
                tracker_sparql_builder_predicate (preupdate, "nco:emailAddress");
                tracker_sparql_builder_object_string (preupdate, email_address);
            tracker_sparql_builder_insert_close(preupdate);

            tracker_sparql_builder_predicate (metadata, predicate);
            tracker_sparql_builder_object_blank_open (metadata);
                tracker_sparql_builder_predicate (metadata, "a");
                tracker_sparql_builder_object (metadata, "nco:Contact");

                if (fullname) {
                    tracker_sparql_builder_predicate (metadata, "nco:fullname");
                    tracker_sparql_builder_object_string (metadata, fullname);
                }

                tracker_sparql_builder_predicate (metadata, "nco:hasEmailAddress");
                tracker_sparql_builder_object_iri (metadata, email_uri);
            tracker_sparql_builder_object_blank_close (metadata);

            g_free (email_uri);
        }
    }
}

static void
extract_message_rfc822 (const gchar             *uri,
                        TrackerSparqlBuilder    *preupdate,
                        TrackerSparqlBuilder    *metadata)
{
    GMimeParser *parser;
    GMimeMessage *message;
    InternetAddressList *ial;
    GMimeHeaderList *headers;
    const gchar *string;
    gchar *buffer;

    g_type_init ();
    g_mime_init (GMIME_ENABLE_RFC2047_WORKAROUNDS);

    parser = _mime_parser_for_uri (uri);

    message = g_mime_parser_construct_message (parser);
    if (! message)
        goto out;

    tracker_sparql_builder_predicate (metadata, "a");
    tracker_sparql_builder_object (metadata, "nmo:Message");

    if (string = g_mime_object_get_header (GMIME_OBJECT (message), "From")) {
        if (ial = internet_address_list_parse_string (string)) {
            if (internet_address_list_length (ial) == 1)
                _mark_addresses_as (preupdate, metadata, ial, "nmo:from");
            g_object_unref (ial);
        }
    }

    if (string = g_mime_object_get_header (GMIME_OBJECT (message), "In-Reply-To")) {
        GMimeReferences *reference, *references;

        references = g_mime_references_decode (string);
        for (reference = references; reference; reference = reference->next) {
            tracker_sparql_builder_predicate (metadata, "nmo:inReplyTo");
            tracker_sparql_builder_object_blank_open (metadata);
                tracker_sparql_builder_predicate (metadata, "a");
                tracker_sparql_builder_object (metadata, "nmo:Message");

                tracker_sparql_builder_predicate (metadata, "nmo:messageId");
                tracker_sparql_builder_object_string (metadata, reference->msgid);
            tracker_sparql_builder_object_blank_close (metadata);
        }
        g_mime_references_clear (&references);
    }

    if (string = g_mime_message_get_message_id (message)) {
        tracker_sparql_builder_predicate (metadata, "nmo:messageId");
        tracker_sparql_builder_object_string (metadata, string);
    }

    if (string = g_mime_message_get_subject (message)) {
        tracker_sparql_builder_predicate (metadata, "nmo:messageSubject");
        tracker_sparql_builder_object_string (metadata, string);
    }

    if (ial = g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_TO)) {
        /* tracker does not have nmo:primaryRecipient */
        _mark_addresses_as (preupdate, metadata, ial, "nmo:to");
        g_object_unref (ial);
    }

    if (string = g_mime_message_get_reply_to (message)) {
        if (ial = internet_address_list_parse_string (string)) {
            if (internet_address_list_length (ial) == 1)
                _mark_addresses_as (preupdate, metadata, ial, "nmo:replyTo");
            g_object_unref (ial);
        }
    }

    if (ial = g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_CC)) {
        /* tracker does not have nmo:secondaryRecipient */
        _mark_addresses_as (preupdate, metadata, ial, "nmo:cc");
        g_object_unref (ial);
    }

    if (ial = g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_BCC)) {
        _mark_addresses_as (preupdate, metadata, ial, "nmo:bcc");
        g_object_unref (ial);
    }

    if (buffer = g_mime_message_get_date_as_string (message)) {
        gchar *sent_date;
        if (sent_date = tracker_date_guess (buffer)) {
            tracker_sparql_builder_predicate (metadata, "nmo:sentDate");
            tracker_sparql_builder_object_string (metadata, sent_date);
            g_free (sent_date);
        }
        g_free (buffer);
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

