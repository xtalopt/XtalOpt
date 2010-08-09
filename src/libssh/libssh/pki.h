#ifndef PKI_H_
#define PKI_H_

#define SSH_KEY_FLAG_EMPTY 0
#define SSH_KEY_FLAG_PUBLIC  1
#define SSH_KEY_FLAG_PRIVATE 2

struct ssh_key_struct {
    enum ssh_keytypes_e type;
    int flags;
    const char *type_c; /* Don't free it ! it is static */
#ifdef HAVE_LIBGCRYPT
    gcry_sexp_t dsa;
    gcry_sexp_t rsa;
#elif HAVE_LIBCRYPTO
    DSA *dsa;
    RSA *rsa;
#endif
};

ssh_key ssh_key_new (void);
void ssh_key_clean (ssh_key key);
enum ssh_keytypes_e ssh_key_type(ssh_key key);
int ssh_key_import_private(ssh_key key, ssh_session session,
    const char *filename, const char *passphrase);
void ssh_key_free (ssh_key key);

#endif /* PKI_H_ */
