/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef SESSION_H_
#define SESSION_H_
#include "libssh/priv.h"
#include "libssh/packet.h"
#include "libssh/pcap.h"
#include "libssh/auth.h"
#include "libssh/channels.h"

typedef struct ssh_kbdint_struct* ssh_kbdint;

/* These are the different states a SSH session can be into its life */
enum ssh_session_state_e {
	SSH_SESSION_STATE_NONE=0,
	SSH_SESSION_STATE_CONNECTING,
	SSH_SESSION_STATE_SOCKET_CONNECTED,
	SSH_SESSION_STATE_BANNER_RECEIVED,
	SSH_SESSION_STATE_INITIAL_KEX,
	SSH_SESSION_STATE_KEXINIT_RECEIVED,
	SSH_SESSION_STATE_DH,
	SSH_SESSION_STATE_AUTHENTICATING,
	SSH_SESSION_STATE_AUTHENTICATED,
	SSH_SESSION_STATE_ERROR,
	SSH_SESSION_STATE_DISCONNECTED
};

enum ssh_dh_state_e {
  DH_STATE_INIT,
  DH_STATE_INIT_SENT,
  DH_STATE_NEWKEYS_SENT,
  DH_STATE_FINISHED
};

struct ssh_session_struct {
    struct error_struct error;
    struct ssh_socket_struct *socket;
    char *serverbanner;
    char *clientbanner;
    int protoversion;
    int server;
    int client;
    int openssh;
    uint32_t send_seq;
    uint32_t recv_seq;
/* status flags */
    int closed;
    int closed_by_except;

    int connected;
    /* !=0 when the user got a session handle */
    int alive;
    /* two previous are deprecated */
    /* int auth_service_asked; */

/* socket status */
    int blocking; /* functions should block */

    ssh_string banner; /* that's the issue banner from
                       the server */
    char *discon_msg; /* disconnect message from
                         the remote host */
    ssh_buffer in_buffer;
    PACKET in_packet;
    ssh_buffer out_buffer;

    /* the states are used by the nonblocking stuff to remember */
    /* where it was before being interrupted */
    enum ssh_session_state_e session_state;
    int packet_state;
    int dh_handshake_state;
    enum ssh_auth_service_state_e auth_service_state;
    enum ssh_auth_state_e auth_state;
    enum ssh_channel_request_state_e global_req_state;
    ssh_string dh_server_signature; /* information used by dh_handshake. */
    KEX server_kex;
    KEX client_kex;
    ssh_buffer in_hashbuf;
    ssh_buffer out_hashbuf;
    struct ssh_crypto_struct *current_crypto;
    struct ssh_crypto_struct *next_crypto;  /* next_crypto is going to be used after a SSH2_MSG_NEWKEYS */

    ssh_channel channels; /* linked list of channels */
    int maxchannel;
    int exec_channel_opened; /* version 1 only. more
                                info in channels1.c */
    ssh_agent agent; /* ssh agent */

/* keyb interactive data */
    struct ssh_kbdint_struct *kbdint;
    int version; /* 1 or 2 */
    /* server host keys */
    ssh_private_key rsa_key;
    ssh_private_key dsa_key;
    /* auths accepted by server */
    int auth_methods;
    int hostkeys; /* contains type of host key wanted by client, in server impl */
    struct ssh_list *ssh_message_list; /* list of delayed SSH messages */
    int (*ssh_message_callback)( struct ssh_session_struct *session, ssh_message msg);
    int log_verbosity; /*cached copy of the option structure */
    int log_indent; /* indentation level in enter_function logs */

    ssh_callbacks callbacks; /* Callbacks to user functions */
    struct ssh_packet_callbacks_struct default_packet_callbacks;
    struct ssh_list *packet_callbacks;
    struct ssh_socket_callbacks_struct socket_callbacks;
    /* options */
#ifdef WITH_PCAP
    ssh_pcap_context pcap_ctx; /* pcap debugging context */
#endif
    char *username;
    char *host;
    char *bindaddr; /* bind the client to an ip addr */
    char *xbanner; /* TODO: looks like it is not needed */
    struct ssh_list *identity;
    char *sshdir;
    char *knownhosts;
    char *wanted_methods[10];
    unsigned long timeout; /* seconds */
    unsigned long timeout_usec;
    unsigned int port;
    socket_t fd;
    int ssh2;
    int ssh1;
    int StrictHostKeyChecking;
    char *ProxyCommand;
};

int ssh_handle_packets(ssh_session session, int timeout);

#endif /* SESSION_H_ */
