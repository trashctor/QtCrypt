#ifndef MYCRYPT_H
#define MYCRYPT_H

#include <QString>

int myEncryptFile(const QString &des_path, const QString &src_path, const QString &key);
int myDecryptFile(const QString &des_path, const QString &src_path, const QString &key, QString
	*decrypt_name = nullptr);

namespace MyCryptPublic
{
	enum : int {SRC_NOT_FOUND, SRC_CANNOT_OPEN_READ, PASS_HASH_FAIL, DES_HEADER_ENCRYPT_ERROR,
		DES_FILE_EXISTS, DES_CANNOT_OPEN_WRITE, DES_HEADER_WRITE_ERROR, SRC_BODY_READ_ERROR,
		DATA_ENCRYPT_ERROR, DES_BODY_WRITE_ERROR, SRC_HEADER_READ_ERROR, SRC_HEADER_DECRYPT_ERROR,
		SRC_FILE_CORRUPT, DATA_DECRYPT_ERROR, CRYPT_SUCCESS};
}

#endif // MYCRYPT_H
