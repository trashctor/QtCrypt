#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>

#include <random>
#include <chrono>
#include <memory>
#include <cstring>
#include <algorithm>

#include "crypto_secretbox.h"
#include "crypto_pwhash_scryptsalsa208sha256.h"
#include "randombytes.h"
#include "mycrypt.h"

#define BUFFER_SIZE 16384
#define BUFFER_SIZE_WITHOUT_MAC (BUFFER_SIZE - crypto_secretbox_MACBYTES)

using namespace MyCryptPublic;


/*******************************************************************************



*******************************************************************************/


int myEncryptFile(const QString &des_path, const QString &src_path, const QString &key)
{
	unsigned char main_buffer[BUFFER_SIZE];
	char pass_buffer[crypto_secretbox_KEYBYTES];
	unsigned char key_buffer[crypto_secretbox_KEYBYTES];
	unsigned char nonce_buffer[crypto_secretbox_NONCEBYTES];
	unsigned char salt_buffer[crypto_pwhash_scryptsalsa208sha256_SALTBYTES];
	qint64 len;

	QFile src_file(QDir::cleanPath(src_path));

	if(!src_file.exists())
		return SRC_NOT_FOUND;

	QFileInfo src_info(src_file);

	if (!src_file.open(QIODevice::ReadOnly))
		return SRC_CANNOT_OPEN_READ;

	// form the key, it stays static for the rest of the encryption
	memset(pass_buffer, 0, crypto_secretbox_KEYBYTES);
	memmove(pass_buffer, key.toUtf8().constData(), std::min(key.toUtf8().size(),
		static_cast<int>(crypto_secretbox_KEYBYTES)));

	randombytes_buf(salt_buffer, crypto_pwhash_scryptsalsa208sha256_SALTBYTES);

	if(crypto_pwhash_scryptsalsa208sha256(key_buffer, crypto_secretbox_KEYBYTES, pass_buffer,
		crypto_secretbox_KEYBYTES, salt_buffer, crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE,
		crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE) != 0)
		return PASS_HASH_FAIL;

	// encrypt just the filename, without the path

	QByteArray src_name = src_info.fileName().toUtf8();

	// move the name data into the buffer

	len = std::min(src_name.size(), static_cast<int>(BUFFER_SIZE_WITHOUT_MAC));
	memmove(main_buffer + crypto_secretbox_MACBYTES, src_name.constData(), len);

	randombytes_buf(nonce_buffer, crypto_secretbox_NONCEBYTES);

	if(crypto_secretbox_easy(main_buffer, main_buffer + crypto_secretbox_MACBYTES, len,
		nonce_buffer, key_buffer) != 0)
		return DES_HEADER_ENCRYPT_ERROR;

	// create the new file, the path should normally be to the same directory as the source
	QFile des_file(QDir::cleanPath(des_path));

	if(des_file.exists())
		return DES_FILE_EXISTS;

	if(!des_file.open(QIODevice::WriteOnly))
		return DES_CANNOT_OPEN_WRITE;

	QDataStream des_stream(&des_file);

	// dump the encrypted filename inside the file along with the nonce and salt
	if(des_stream.writeRawData(reinterpret_cast<char *>(nonce_buffer), crypto_secretbox_NONCEBYTES)
		< crypto_secretbox_NONCEBYTES)
		return DES_HEADER_WRITE_ERROR;

	if(des_stream.writeRawData(reinterpret_cast<char *>(salt_buffer),
		crypto_pwhash_scryptsalsa208sha256_SALTBYTES) < crypto_pwhash_scryptsalsa208sha256_SALTBYTES)
		return DES_HEADER_WRITE_ERROR;

	qint64 temp_qint64 = len + crypto_secretbox_MACBYTES;

	if(des_stream.writeRawData(reinterpret_cast<char *>(&temp_qint64), sizeof(qint64)) <
		sizeof(qint64))
		return DES_HEADER_WRITE_ERROR;

	if(des_stream.writeRawData(reinterpret_cast<char *>(main_buffer), temp_qint64) < temp_qint64)
		return DES_HEADER_WRITE_ERROR;

	// now encrypt the actual data
	QDataStream src_stream(&src_file);

	while(1)
	{
		len = src_stream.readRawData(reinterpret_cast<char *>(main_buffer + crypto_secretbox_MACBYTES),
			BUFFER_SIZE_WITHOUT_MAC);
		//memset(main_buffer, 0, crypto_secretbox_MACBYTES);

		if(len == -1)
			return SRC_BODY_READ_ERROR;
		else if(len == 0)
			break;

		// get a new nonce and encrypt
		randombytes_buf(nonce_buffer, crypto_secretbox_NONCEBYTES);
		if(crypto_secretbox_easy(main_buffer, main_buffer + crypto_secretbox_MACBYTES, len,
			nonce_buffer, key_buffer) != 0)
			return DATA_ENCRYPT_ERROR;

		if(des_stream.writeRawData(reinterpret_cast<char *>(nonce_buffer), crypto_secretbox_NONCEBYTES)
			< crypto_secretbox_NONCEBYTES)
			return DES_BODY_WRITE_ERROR;

		if(des_stream.writeRawData(reinterpret_cast<char *>(main_buffer), len +
			crypto_secretbox_MACBYTES) < len + crypto_secretbox_MACBYTES)
			return DES_BODY_WRITE_ERROR;
	}

	return CRYPT_SUCCESS;
}


/*******************************************************************************



*******************************************************************************/


int myDecryptFile(const QString &des_path, const QString &src_path, const QString &key, QString
	*decrypt_name)
{
	unsigned char main_buffer[BUFFER_SIZE];
	unsigned char key_buffer[crypto_secretbox_KEYBYTES];
	char pass_buffer[crypto_secretbox_KEYBYTES];
	unsigned char nonce_buffer[crypto_secretbox_NONCEBYTES];
	unsigned char salt_buffer[crypto_pwhash_scryptsalsa208sha256_SALTBYTES];
	qint64 len;

	QFile src_file(QDir::cleanPath(src_path));

	if(!src_file.exists())
		return SRC_NOT_FOUND;

	if (!src_file.open(QIODevice::ReadOnly))
		return SRC_CANNOT_OPEN_READ;

	// open the encrypted file and extract the header data, including the filename and password salt
	QDataStream src_stream(&src_file);

	if(src_stream.readRawData(reinterpret_cast<char *>(nonce_buffer), crypto_secretbox_NONCEBYTES)
		< crypto_secretbox_NONCEBYTES)
		return SRC_HEADER_READ_ERROR;

	if(src_stream.readRawData(reinterpret_cast<char *>(salt_buffer),
		crypto_pwhash_scryptsalsa208sha256_SALTBYTES) < crypto_pwhash_scryptsalsa208sha256_SALTBYTES)
		return SRC_HEADER_READ_ERROR;

	if(src_stream.readRawData(reinterpret_cast<char *>(&len), sizeof(qint64)) < sizeof(qint64))
		return SRC_HEADER_READ_ERROR;

	if(src_stream.readRawData(reinterpret_cast<char *>(main_buffer), len) < len)
		return SRC_HEADER_READ_ERROR;

	// form the key, it stays static for the rest of the encryption
	memset(pass_buffer, 0, crypto_secretbox_KEYBYTES);
	memmove(pass_buffer, key.toUtf8().constData(), std::min(key.toUtf8().size(),
		static_cast<int>(crypto_secretbox_KEYBYTES)));

	if(crypto_pwhash_scryptsalsa208sha256(key_buffer, crypto_secretbox_KEYBYTES, pass_buffer,
		crypto_secretbox_KEYBYTES, salt_buffer, crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE,
		crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE) != 0)
		return PASS_HASH_FAIL;

	// decrypt the filename data in the main buffer

	if(crypto_secretbox_open_easy(main_buffer + crypto_secretbox_MACBYTES, main_buffer, len,
		nonce_buffer, key_buffer) != 0)
		return SRC_HEADER_DECRYPT_ERROR;

	// get the new file name
	QString des_name = QString::fromUtf8(reinterpret_cast<char *>(main_buffer +
		crypto_secretbox_MACBYTES), len - crypto_secretbox_MACBYTES);

	// return the name of the decrypted file
	if(decrypt_name != nullptr)
		*decrypt_name = des_name;

	// create the decrypted file in the passed directory
	QFile des_file(QDir::cleanPath(des_path + "/" + des_name));

	if(des_file.exists())
		return DES_FILE_EXISTS;

	if(!des_file.open(QIODevice::WriteOnly))
		return DES_CANNOT_OPEN_WRITE;

	QDataStream des_stream(&des_file);

	// start decrypting the actual data
	while(1)
	{
		len = src_stream.readRawData(reinterpret_cast<char *>(nonce_buffer),
			crypto_secretbox_NONCEBYTES);

		if(len == 0)
			break;
		else if(len == -1)
			return SRC_BODY_READ_ERROR;
		else if(len < crypto_secretbox_NONCEBYTES)
			return SRC_FILE_CORRUPT;

		len = src_stream.readRawData(reinterpret_cast<char *>(main_buffer), BUFFER_SIZE);

		if(len == -1)
			return SRC_BODY_READ_ERROR;
		else if(len == 0)
			return SRC_FILE_CORRUPT;

		if(crypto_secretbox_open_easy(main_buffer + crypto_secretbox_MACBYTES, main_buffer, len,
			nonce_buffer, key_buffer) != 0)
			return DATA_DECRYPT_ERROR;

		if(des_stream.writeRawData(reinterpret_cast<char *>(main_buffer + crypto_secretbox_MACBYTES),
			len - crypto_secretbox_MACBYTES) < len - crypto_secretbox_MACBYTES)
			return DES_BODY_WRITE_ERROR;
	}

	return CRYPT_SUCCESS;
}
