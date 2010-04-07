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
_parse_address (const gchar *address,
                gchar       **fullname,
                gchar       **email_address)
{
    InternetAddress *ia;
    InternetAddressList *ial;

    g_return_if_fail (fullname);
    g_return_if_fail (email_address);

    *fullname = *email_address = NULL;

    ial = internet_address_list_parse_string (address);
    if (! ial)
        return;

    /* the address contains more than a single entity */
    if (internet_address_list_length (ial) != 1) {
        g_object_unref (ial);
        return;
    }

    ia = internet_address_list_get_address (ial, 0);

    *fullname = g_strdup (internet_address_get_name (ia));

    if (INTERNET_ADDRESS_IS_MAILBOX (ia)) {
        *email_address = g_strdup (internet_address_mailbox_get_addr (INTERNET_ADDRESS_MAILBOX (ia)));
    }

    g_object_unref (ial);
}

static void
extract_message_rfc822 (const gchar             *uri,
                        TrackerSparqlBuilder    *preupdate,
                        TrackerSparqlBuilder    *metadata)
{
    GMimeParser *parser;
    GMimeMessage *message;
    InternetAddressList *ial;
    const gchar *header, *subject, *message_id;
    gchar *date;

    g_type_init ();
    g_mime_init (GMIME_ENABLE_RFC2047_WORKAROUNDS);

    parser = _mime_parser_for_uri (uri);

    message = g_mime_parser_construct_message (parser);
    if (! message)
        goto out;

    tracker_sparql_builder_predicate (metadata, "a");
    tracker_sparql_builder_object (metadata, "nmo:Message");

    header = g_mime_object_get_header (GMIME_OBJECT (message), "From");
    if (header) {
        gchar *fullname = NULL, *email_address = NULL, *email_uri = NULL;

        _parse_address (header, &fullname, &email_address);

        if (email_address) {
            email_uri = tracker_uri_printf_escaped ("mailto:%s", email_address);

            tracker_sparql_builder_subject_iri (metadata, email_uri);
            tracker_sparql_builder_predicate (metadata, "rdf:type");
            tracker_sparql_builder_object (metadata, "nco:EmailAddress");

            tracker_sparql_builder_subject_iri (metadata, email_uri);
            tracker_sparql_builder_predicate (metadata, "nco:emailAddress");
            tracker_sparql_builder_object_string (metadata, email_address);

            g_free (email_address);
        }

        tracker_sparql_builder_predicate (metadata, "nmo:from");

        tracker_sparql_builder_object_blank_open (metadata);
        tracker_sparql_builder_predicate (metadata, "rdf:type");
        tracker_sparql_builder_object (metadata, "nco:Contact");

        if (fullname) {
            tracker_sparql_builder_predicate (metadata, "nco:fullname");
            tracker_sparql_builder_object_string (metadata, fullname);
            g_free (fullname);
        }

        if (email_uri) {
            tracker_sparql_builder_predicate (metadata, "nco:hasEmailAddress");
            tracker_sparql_builder_object_iri (metadata, email_uri);
            g_free (email_uri);
        }

        tracker_sparql_builder_object_blank_close (metadata);
    }

    message_id = g_mime_message_get_message_id (message);
    if (message_id) {
        tracker_sparql_builder_predicate (metadata, "nmo:messageId");
        tracker_sparql_builder_object_string (metadata, message_id);
    }

    subject = g_mime_message_get_subject (message);
    if (subject) {
        tracker_sparql_builder_predicate (metadata, "nmo:messageSubject");
        tracker_sparql_builder_object_string (metadata, subject);
    }

    ial = g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_TO);
    if (ial) {
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

                tracker_sparql_builder_subject_iri (metadata, email_uri);
                tracker_sparql_builder_predicate (metadata, "rdf:type");
                tracker_sparql_builder_object (metadata, "nco:EmailAddress");

                tracker_sparql_builder_subject_iri (metadata, email_uri);
                tracker_sparql_builder_predicate (metadata, "nco:emailAddress");
                tracker_sparql_builder_object_string (metadata, email_address);
            }

            tracker_sparql_builder_predicate (metadata, "nmo:primaryRecipient");

            tracker_sparql_builder_object_blank_open (metadata);
            tracker_sparql_builder_predicate (metadata, "rdf:typ");
            tracker_sparql_builder_object (metadata, "nco:Contact");

            if (fullname) {
                tracker_sparql_builder_predicate (metadata, "nco:fullname");
                tracker_sparql_builder_object_string (metadata, fullname);
            }

            if (email_uri) {
                tracker_sparql_builder_predicate (metadata, "nco:hasEmailAddress");
                tracker_sparql_builder_object_iri (metadata, email_uri);
                g_free (email_uri);
            }

            tracker_sparql_builder_object_blank_close (metadata);
        }

        g_object_unref (ial);
    }

    header = g_mime_message_get_reply_to (message);
    if (header) {
        gchar *fullname = NULL, *email_address = NULL, *email_uri = NULL;

        _parse_address (header, &fullname, &email_address);

        if (email_address) {
            email_uri = tracker_uri_printf_escaped ("mailto:%s", email_address);

            tracker_sparql_builder_subject_iri (metadata, email_uri);
            tracker_sparql_builder_predicate (metadata, "rdf:type");
            tracker_sparql_builder_object (metadata, "nco:EmailAddress");

            tracker_sparql_builder_subject_iri (metadata, email_uri);
            tracker_sparql_builder_predicate (metadata, "nco:emailAddress");
            tracker_sparql_builder_object_string (metadata, email_address);

            g_free (email_address);
        }

        tracker_sparql_builder_predicate (metadata, "nmo:replyTo");

        tracker_sparql_builder_object_blank_open (metadata);
        tracker_sparql_builder_predicate (metadata, "rdf:type");
        tracker_sparql_builder_object (metadata, "nco:Contact");

        if (fullname) {
            tracker_sparql_builder_predicate (metadata, "nco:fullname");
            tracker_sparql_builder_object_string (metadata, fullname);
            g_free (fullname);
        }

        if (email_uri) {
            tracker_sparql_builder_predicate (metadata, "nco:hasEmailAddress");
            tracker_sparql_builder_object_iri (metadata, email_uri);
            g_free (email_uri);
        }

        tracker_sparql_builder_object_blank_close (metadata);
    }

    ial = g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_CC);
    if (ial) {
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

                tracker_sparql_builder_subject_iri (metadata, email_uri);
                tracker_sparql_builder_predicate (metadata, "rdf:type");
                tracker_sparql_builder_object (metadata, "nco:EmailAddress");

                tracker_sparql_builder_subject_iri (metadata, email_uri);
                tracker_sparql_builder_predicate (metadata, "nco:emailAddress");
                tracker_sparql_builder_object_string (metadata, email_address);
            }

            tracker_sparql_builder_predicate (metadata, "nmo:secondaryRecipient");

            tracker_sparql_builder_object_blank_open (metadata);
            tracker_sparql_builder_predicate (metadata, "rdf:typ");
            tracker_sparql_builder_object (metadata, "nco:Contact");

            if (fullname) {
                tracker_sparql_builder_predicate (metadata, "nco:fullname");
                tracker_sparql_builder_object_string (metadata, fullname);
            }

            if (email_uri) {
                tracker_sparql_builder_predicate (metadata, "nco:hasEmailAddress");
                tracker_sparql_builder_object_iri (metadata, email_uri);
                g_free (email_uri);
            }

            tracker_sparql_builder_object_blank_close (metadata);
        }

        g_object_unref (ial);
    }

    date = g_mime_message_get_date_as_string (message);
    if (date) {
        gchar *parsed;

        parsed = tracker_date_guess (date);
        if (parsed) {
            tracker_sparql_builder_predicate (metadata, "nmo:sentDate");
            tracker_sparql_builder_object_string (metadata, parsed);
            g_free (parsed);
        }

        g_free (date);
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

