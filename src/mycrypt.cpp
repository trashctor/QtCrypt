#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <QtEndian>

#include "crypto_secretbox.h"
#include "crypto_pwhash_scryptsalsa208sha256.h"
#include "randombytes.h"

#include "mycrypt.h"

#define BUFFER_SIZE 16384
#define BUFFER_SIZE_WITHOUT_MAC (BUFFER_SIZE - crypto_secretbox_MACBYTES)

using namespace MyCryptPublic;


/*******************************************************************************



*******************************************************************************/


void myIncrementNonce(unsigned char *nonce, int nonce_size)
{
	for(int i = 0; i < nonce_size; i++)
	{
		nonce[i]++;

		if(nonce[i] != '\0')
			break;
	}
}


/*******************************************************************************



*******************************************************************************/


int myEncryptFile(const QString &des_path, const QString &src_path, const QString &key)
{
	unsigned char main_buffer[BUFFER_SIZE];
	char pass_buffer[crypto_secretbox_KEYBYTES];
	unsigned char key_buffer[crypto_secretbox_KEYBYTES];
	unsigned char nonce_buffer[crypto_secretbox_NONCEBYTES];
	unsigned char salt_buffer[crypto_pwhash_scryptsalsa208sha256_SALTBYTES];
	unsigned char qint64_buffer[crypto_secretbox_MACBYTES + sizeof(qint64)];
	qint64 len;

	QFile src_file(QDir::cleanPath(src_path));
	QFileInfo src_info(src_file);

	if(!src_file.exists() || !src_info.isFile())
		return SRC_NOT_FOUND;

	if (!src_file.open(QIODevice::ReadOnly))
		return SRC_CANNOT_OPEN_READ;

	// form the key, it stays static for the rest of the encryption
	memset(pass_buffer, 0, crypto_secretbox_KEYBYTES);
	memcpy(pass_buffer, key.toUtf8().constData(), std::min(key.toUtf8().size(),
		static_cast<int>(crypto_secretbox_KEYBYTES)));

	randombytes_buf(salt_buffer, crypto_pwhash_scryptsalsa208sha256_SALTBYTES);

	if(crypto_pwhash_scryptsalsa208sha256(key_buffer, crypto_secretbox_KEYBYTES, pass_buffer,
		crypto_secretbox_KEYBYTES, salt_buffer, crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE,
		crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE) != 0)
		return PASS_HASH_FAIL;

	// create the new file, the path should normally be to the same directory as the source
	QFile des_file(QDir::cleanPath(des_path));

	if(des_file.exists())
		return DES_FILE_EXISTS;

	if(!des_file.open(QIODevice::WriteOnly))
		return DES_CANNOT_OPEN_WRITE;

	QDataStream des_stream(&des_file);

	// dump the nonce and salt in first
	randombytes_buf(nonce_buffer, crypto_secretbox_NONCEBYTES);

	if(des_stream.writeRawData(reinterpret_cast<char *>(nonce_buffer), crypto_secretbox_NONCEBYTES)
		< crypto_secretbox_NONCEBYTES)
		return DES_HEADER_WRITE_ERROR;

	if(des_stream.writeRawData(reinterpret_cast<char *>(salt_buffer),
		crypto_pwhash_scryptsalsa208sha256_SALTBYTES) < crypto_pwhash_scryptsalsa208sha256_SALTBYTES)
		return DES_HEADER_WRITE_ERROR;

	// move the file size and name data into the main buffer
	QByteArray src_name = src_info.fileName().toUtf8();

	qint64 filesize = src_info.size();
	qToLittleEndian<qint64>(filesize, main_buffer + crypto_secretbox_MACBYTES);

	len = std::min(static_cast<unsigned int>(src_name.size()), BUFFER_SIZE_WITHOUT_MAC -
		sizeof(qint64));
	memcpy(main_buffer + crypto_secretbox_MACBYTES + sizeof(qint64), src_name.constData(), len);

	// next, encode the header size and put it into the destination
	qint64 header_size = crypto_secretbox_MACBYTES + sizeof(qint64) + len;
	qToLittleEndian<qint64>(header_size, qint64_buffer + crypto_secretbox_MACBYTES);

	if(crypto_secretbox_easy(qint64_buffer, qint64_buffer +	crypto_secretbox_MACBYTES, sizeof(qint64),
		nonce_buffer,	key_buffer) != 0)
		return DES_HEADER_ENCRYPT_ERROR;

	if(des_stream.writeRawData(reinterpret_cast<char *>(qint64_buffer), crypto_secretbox_MACBYTES +
		sizeof(qint64)) <	crypto_secretbox_MACBYTES + sizeof(qint64))
		return DES_HEADER_WRITE_ERROR;

	// finally, encrypt and write in the file size and filename
	myIncrementNonce(nonce_buffer, crypto_secretbox_NONCEBYTES);

	if(crypto_secretbox_easy(main_buffer, main_buffer +	crypto_secretbox_MACBYTES, header_size -
		crypto_secretbox_MACBYTES, nonce_buffer, key_buffer) != 0)
		return DES_HEADER_ENCRYPT_ERROR;

	if(des_stream.writeRawData(reinterpret_cast<char *>(main_buffer), header_size) < header_size)
		return DES_HEADER_WRITE_ERROR;

	// now, move on to the actual data
	QDataStream src_stream(&src_file);

	qint64 curr_read = 0;

	while(1)
	{
		len = src_stream.readRawData(reinterpret_cast<char *>(main_buffer + crypto_secretbox_MACBYTES),
			BUFFER_SIZE_WITHOUT_MAC);
		curr_read += len;

		if(len == -1)
			return SRC_BODY_READ_ERROR;
		else if(len == 0)
		{
			if(curr_read != filesize)
				return SRC_BODY_READ_ERROR;
			else
				break;
		}
		else if(curr_read > filesize)
			return SRC_BODY_READ_ERROR;

		// increment the nonce, encrypt and write the cipertext to the destination file
		myIncrementNonce(nonce_buffer, crypto_secretbox_NONCEBYTES);

		if(crypto_secretbox_easy(main_buffer, main_buffer + crypto_secretbox_MACBYTES, len,
			nonce_buffer, key_buffer) != 0)
			return DATA_ENCRYPT_ERROR;

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
	unsigned char qint64_buffer[crypto_secretbox_MACBYTES + sizeof(qint64)];
	qint64 len;

	QFile src_file(QDir::cleanPath(src_path));
	QFileInfo src_info(src_file);

	if(!src_file.exists() || !src_info.isFile())
		return SRC_NOT_FOUND;

	if (!src_file.open(QIODevice::ReadOnly))
		return SRC_CANNOT_OPEN_READ;

	// open the source file and extract the initial nonce and password salt
	QDataStream src_stream(&src_file);

	if(src_stream.readRawData(reinterpret_cast<char *>(nonce_buffer), crypto_secretbox_NONCEBYTES)
		< crypto_secretbox_NONCEBYTES)
		return SRC_HEADER_READ_ERROR;

	if(src_stream.readRawData(reinterpret_cast<char *>(salt_buffer),
		crypto_pwhash_scryptsalsa208sha256_SALTBYTES) < crypto_pwhash_scryptsalsa208sha256_SALTBYTES)
		return SRC_HEADER_READ_ERROR;

	// form the key, it stays static for the rest of the encryption
	memset(pass_buffer, 0, crypto_secretbox_KEYBYTES);
	memcpy(pass_buffer, key.toUtf8().constData(), std::min(key.toUtf8().size(),
		static_cast<int>(crypto_secretbox_KEYBYTES)));

	if(crypto_pwhash_scryptsalsa208sha256(key_buffer, crypto_secretbox_KEYBYTES, pass_buffer,
		crypto_secretbox_KEYBYTES, salt_buffer, crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE,
		crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE) != 0)
		return PASS_HASH_FAIL;

	// next, get the encrypted header size and decrypt it
	qint64 header_size;

	if(src_stream.readRawData(reinterpret_cast<char *>(qint64_buffer), crypto_secretbox_MACBYTES +
		sizeof(qint64)) <	crypto_secretbox_MACBYTES + sizeof(qint64))
		return SRC_HEADER_READ_ERROR;

	if(crypto_secretbox_open_easy(qint64_buffer + crypto_secretbox_MACBYTES, qint64_buffer,
		crypto_secretbox_MACBYTES + sizeof(qint64),	nonce_buffer, key_buffer) != 0)
		return SRC_HEADER_DECRYPT_ERROR;

	header_size = qFromLittleEndian<qint64>(qint64_buffer +	crypto_secretbox_MACBYTES);

	if(header_size <= crypto_secretbox_MACBYTES + sizeof(qint64) || header_size > BUFFER_SIZE)
		return SRC_HEADER_DECRYPT_ERROR;

	if(src_stream.readRawData(reinterpret_cast<char *>(main_buffer), header_size) < header_size)
		return SRC_HEADER_READ_ERROR;

	// get the file size and filename of original item
	myIncrementNonce(nonce_buffer, crypto_secretbox_NONCEBYTES);

	if(crypto_secretbox_open_easy(main_buffer + crypto_secretbox_MACBYTES, main_buffer, header_size,
		nonce_buffer, key_buffer) != 0)
		return SRC_HEADER_DECRYPT_ERROR;

	qint64 filesize = qFromLittleEndian<qint64>(main_buffer + crypto_secretbox_MACBYTES);
	len = header_size - crypto_secretbox_MACBYTES - sizeof(qint64);

	QString des_name = QString::fromUtf8(reinterpret_cast<char *>(main_buffer +
		crypto_secretbox_MACBYTES + sizeof(qint64)), len);

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

	qint64 curr_read = 0;

	// start decrypting the actual data
	while(1)
	{
		// curr_read is the cumulative amount of the ciphertext (without the MAC) read so far
		len = src_stream.readRawData(reinterpret_cast<char *>(main_buffer), BUFFER_SIZE);
		curr_read += std::max(len - crypto_secretbox_MACBYTES, static_cast<qint64>(0));

		if(len == -1)
			return SRC_BODY_READ_ERROR;
		else if(len == 0)
		{
			if(curr_read != filesize)
				return SRC_FILE_CORRUPT;
			else
				break;
		}
		else if(len <= crypto_secretbox_MACBYTES)
			return SRC_FILE_CORRUPT;
		else if(len < BUFFER_SIZE)
		{
			if(curr_read != filesize)
				return SRC_FILE_CORRUPT;
		}
		else if(curr_read > filesize)
			return SRC_FILE_CORRUPT;

		// increment the nonce, decrypt and write the plaintext block to the destination
		myIncrementNonce(nonce_buffer, crypto_secretbox_NONCEBYTES);

		if(crypto_secretbox_open_easy(main_buffer + crypto_secretbox_MACBYTES, main_buffer, len,
			nonce_buffer, key_buffer) != 0)
			return DATA_DECRYPT_ERROR;

		if(des_stream.writeRawData(reinterpret_cast<char *>(main_buffer + crypto_secretbox_MACBYTES),
			len - crypto_secretbox_MACBYTES) < len - crypto_secretbox_MACBYTES)
			return DES_BODY_WRITE_ERROR;
	}

	return CRYPT_SUCCESS;
}
