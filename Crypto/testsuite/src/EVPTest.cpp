//
// EVPTest.cpp
//
// Copyright (c) 2008, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "EVPTest.h"
#include "Poco/Crypto/RSAKey.h"
#include "Poco/Crypto/ECKey.h"
#include "Poco/Crypto/EVPPKey.h"
#include "Poco/TemporaryFile.h"
#include "Poco/StreamCopier.h"
#include "Poco/CppUnit/TestCaller.h"
#include "Poco/CppUnit/TestSuite.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstring>


using namespace Poco::Crypto;
using Poco::TemporaryFile;
using Poco::StreamCopier;


EVPTest::EVPTest(const std::string& name): CppUnit::TestCase(name)
{
}


EVPTest::~EVPTest()
{
}


void EVPTest::testRSAEVPPKey()
{
	try
	{
		RSAKey* key = new RSAKey(RSAKey::KL_1024, RSAKey::EXP_SMALL);
		assertTrue (key->type() == Poco::Crypto::KeyPair::KT_RSA);
		// construct EVPPKey from RSAKey*
		EVPPKey* pKey = new EVPPKey(key);
		// EVPPKey increments reference count, so freeing the original must be ok
		delete key;

		assertTrue (!pKey->isSupported(0));
		assertTrue (!pKey->isSupported(-1));
		assertTrue (pKey->isSupported(pKey->type()));
		assertTrue (pKey->type() == EVP_PKEY_RSA);

		// construct RSAKey from const EVPPKey&
		key = new RSAKey(*pKey);
		delete pKey;
		assertTrue (key->type() == Poco::Crypto::KeyPair::KT_RSA);
		// construct EVPPKey from RSAKey*
		pKey = new EVPPKey(key);
		assertTrue (pKey->type() == EVP_PKEY_RSA);

		BIO* bioPriv1 = BIO_new(BIO_s_mem());
		BIO* bioPub1 = BIO_new(BIO_s_mem());
		assertTrue (0 != PEM_write_bio_PrivateKey(bioPriv1, *pKey, NULL, NULL, 0, 0, NULL));
		assertTrue (0 != PEM_write_bio_PUBKEY(bioPub1, *pKey));
		char* pPrivData1;
		long sizePriv1 = BIO_get_mem_data(bioPriv1, &pPrivData1);
		char* pPubData1;
		long sizePub1 = BIO_get_mem_data(bioPub1, &pPubData1);

		// construct EVPPKey from EVP_PKEY*
		EVPPKey evpPKey(pKey->operator EVP_PKEY*());
		// EVPPKey makes duplicate, so freeing the original must be ok
		delete pKey;
		assertTrue (evpPKey.type() == EVP_PKEY_RSA);

		BIO* bioPriv2 = BIO_new(BIO_s_mem());
		BIO* bioPub2 = BIO_new(BIO_s_mem());
		assertTrue (0 != PEM_write_bio_PrivateKey(bioPriv2, evpPKey, NULL, NULL, 0, 0, NULL));
		assertTrue (0 != PEM_write_bio_PUBKEY(bioPub2, evpPKey));
		char* pPrivData2;
		long sizePriv2 = BIO_get_mem_data(bioPriv2, &pPrivData2);
		char* pPubData2;
		long sizePub2 = BIO_get_mem_data(bioPub2, &pPubData2);

		assertTrue (sizePriv1 && (sizePriv1 == sizePriv2));
		assertTrue (0 == memcmp(pPrivData1, pPrivData2, sizePriv1));
		assertTrue (sizePub1 && (sizePub1 == sizePub2));
		assertTrue (0 == memcmp(pPubData1, pPubData2, sizePub1));

		BIO_free(bioPub2);
		BIO_free(bioPriv2);

		// copy
		EVPPKey evpPKey2(evpPKey);
		assertTrue (evpPKey2.type() == EVP_PKEY_RSA);
		bioPriv2 = BIO_new(BIO_s_mem());
		bioPub2 = BIO_new(BIO_s_mem());
		assertTrue (0 != PEM_write_bio_PrivateKey(bioPriv2, evpPKey2, NULL, NULL, 0, 0, NULL));
		assertTrue (0 != PEM_write_bio_PUBKEY(bioPub2, evpPKey2));
		sizePriv2 = BIO_get_mem_data(bioPriv2, &pPrivData2);
		sizePub2 = BIO_get_mem_data(bioPub2, &pPubData2);

		assertTrue (sizePriv1 && (sizePriv1 == sizePriv2));
		assertTrue (0 == memcmp(pPrivData1, pPrivData2, sizePriv1));
		assertTrue (sizePub1 && (sizePub1 == sizePub2));
		assertTrue (0 == memcmp(pPubData1, pPubData2, sizePub1));

		BIO_free(bioPub2);
		BIO_free(bioPriv2);

		// move
		EVPPKey evpPKey3(std::move(evpPKey2));
		assertTrue (evpPKey3.type() == EVP_PKEY_RSA);
		bioPriv2 = BIO_new(BIO_s_mem());
		bioPub2 = BIO_new(BIO_s_mem());
		assertTrue (0 != PEM_write_bio_PrivateKey(bioPriv2, evpPKey3, NULL, NULL, 0, 0, NULL));
		assertTrue (0 != PEM_write_bio_PUBKEY(bioPub2, evpPKey3));
		sizePriv2 = BIO_get_mem_data(bioPriv2, &pPrivData2);
		sizePub2 = BIO_get_mem_data(bioPub2, &pPubData2);

		assertTrue (sizePriv1 && (sizePriv1 == sizePriv2));
		assertTrue (0 == memcmp(pPrivData1, pPrivData2, sizePriv1));
		assertTrue (sizePub1 && (sizePub1 == sizePub2));
		assertTrue (0 == memcmp(pPubData1, pPubData2, sizePub1));

		BIO_free(bioPub2);
		BIO_free(bioPriv2);
		BIO_free(bioPub1);
		BIO_free(bioPriv1);
	}
	catch (Poco::Exception& ex)
	{
		std::cerr << ex.displayText() << std::endl;
		throw;
	}
}


void EVPTest::testRSAEVPSaveLoadStream()
{
	RSAKey rsaKey(RSAKey::KL_1024, RSAKey::EXP_SMALL);
	EVPPKey key(&rsaKey);
	std::ostringstream strPub;
	std::ostringstream strPriv;
	key.save(&strPub, &strPriv, "testpwd");
	std::string pubKey = strPub.str();
	std::string privKey = strPriv.str();

	// now do the round trip
	std::istringstream iPub(pubKey);
	std::istringstream iPriv(privKey);
	EVPPKey key2(&iPub, &iPriv, "testpwd");

	assertTrue (key == key2);
	assertTrue (!(key != key2));
	RSAKey rsaKeyNE(RSAKey::KL_1024, RSAKey::EXP_LARGE);
	EVPPKey keyNE(&rsaKeyNE);
	assertTrue (key != keyNE);
	assertTrue (!(key == keyNE));
	assertTrue (key2 != keyNE);;
	assertTrue (!(key2 == keyNE));

	std::ostringstream strPub2;
	std::ostringstream strPriv2;
	key2.save(&strPub2, &strPriv2, "testpwd");
	assertTrue (strPub2.str() == pubKey);

	std::istringstream iPriv2(strPriv2.str());
	EVPPKey key3(0, &iPriv2,  "testpwd");
	std::ostringstream strPub3;
	key3.save(&strPub3);
	assertTrue (strPub3.str() == pubKey);
}


void EVPTest::testRSAEVPSaveLoadStreamNoPass()
{
	RSAKey rsaKey(RSAKey::KL_1024, RSAKey::EXP_SMALL);
	EVPPKey key(&rsaKey);
	std::ostringstream strPub;
	std::ostringstream strPriv;
	key.save(&strPub, &strPriv);
	std::string pubKey = strPub.str();
	std::string privKey = strPriv.str();

	// now do the round trip
	std::istringstream iPub(pubKey);
	std::istringstream iPriv(privKey);
	EVPPKey key2(&iPub, &iPriv);

	assertTrue (key == key2);
	assertTrue (!(key != key2));
	RSAKey rsaKeyNE(RSAKey::KL_1024, RSAKey::EXP_LARGE);
	EVPPKey keyNE(&rsaKeyNE);
	assertTrue (key != keyNE);
	assertTrue (!(key == keyNE));
	assertTrue (key2 != keyNE);;
	assertTrue (!(key2 == keyNE));

	std::istringstream iPriv2(privKey);
	EVPPKey key3(0, &iPriv2);
	std::ostringstream strPub3;
	key3.save(&strPub3);
	std::string pubFromPrivate = strPub3.str();
	assertTrue (pubFromPrivate == pubKey);
}


void EVPTest::testECEVPPKey()
{
	try
	{
		std::string curveName = ECKey::getCurveName();
		if (!curveName.empty())
		{
			EVPPKey*pKey = new EVPPKey(curveName);
			assertTrue (pKey != 0);
			assertTrue (!pKey->isSupported(0));
			assertTrue (!pKey->isSupported(-1));
			assertTrue (pKey->isSupported(pKey->type()));
			assertTrue (pKey->type() == EVP_PKEY_EC);

			BIO*bioPriv1 = BIO_new(BIO_s_mem());
			BIO*bioPub1 = BIO_new(BIO_s_mem());
			assertTrue (0 != PEM_write_bio_PrivateKey(bioPriv1, *pKey, NULL, NULL, 0, 0, NULL));
			assertTrue (0 != PEM_write_bio_PUBKEY(bioPub1, *pKey));
			char*pPrivData1;
			long sizePriv1 = BIO_get_mem_data(bioPriv1, &pPrivData1);
			char*pPubData1;
			long sizePub1 = BIO_get_mem_data(bioPub1, &pPubData1);

			// construct EVPPKey from EVP_PKEY*
			EVPPKey evpPKey(pKey->operator EVP_PKEY*());
			assertTrue (evpPKey.type() == EVP_PKEY_EC);
			// EVPPKey makes duplicate, so freeing the original must be ok
			delete pKey;

			BIO*bioPriv2 = BIO_new(BIO_s_mem());
			BIO*bioPub2 = BIO_new(BIO_s_mem());
			assertTrue (0 != PEM_write_bio_PrivateKey(bioPriv2, evpPKey, NULL, NULL, 0, 0, NULL));
			assertTrue (0 != PEM_write_bio_PUBKEY(bioPub2, evpPKey));
			char*pPrivData2;
			long sizePriv2 = BIO_get_mem_data(bioPriv2, &pPrivData2);
			char*pPubData2;
			long sizePub2 = BIO_get_mem_data(bioPub2, &pPubData2);

			assertTrue (sizePriv1 && (sizePriv1 == sizePriv2));
			assertTrue (0 == memcmp(pPrivData1, pPrivData2, sizePriv1));
			assertTrue (sizePub1 && (sizePub1 == sizePub2));
			assertTrue (0 == memcmp(pPubData1, pPubData2, sizePub1));

			BIO_free(bioPub2);
			BIO_free(bioPriv2);

			// copy
			EVPPKey evpPKey2(evpPKey);
			assertTrue (evpPKey2.type() == EVP_PKEY_EC);
			bioPriv2 = BIO_new(BIO_s_mem());
			bioPub2 = BIO_new(BIO_s_mem());
			assertTrue (0 != PEM_write_bio_PrivateKey(bioPriv2, evpPKey2, NULL, NULL, 0, 0, NULL));
			assertTrue (0 != PEM_write_bio_PUBKEY(bioPub2, evpPKey2));
			sizePriv2 = BIO_get_mem_data(bioPriv2, &pPrivData2);
			sizePub2 = BIO_get_mem_data(bioPub2, &pPubData2);

			assertTrue (sizePriv1 && (sizePriv1 == sizePriv2));
			assertTrue (0 == memcmp(pPrivData1, pPrivData2, sizePriv1));
			assertTrue (sizePub1 && (sizePub1 == sizePub2));
			assertTrue (0 == memcmp(pPubData1, pPubData2, sizePub1));

			BIO_free(bioPub2);
			BIO_free(bioPriv2);

			// move
			EVPPKey evpPKey3(std::move(evpPKey2));
			assertTrue (evpPKey3.type() == EVP_PKEY_EC);
			bioPriv2 = BIO_new(BIO_s_mem());
			bioPub2 = BIO_new(BIO_s_mem());
			assertTrue (0 != PEM_write_bio_PrivateKey(bioPriv2, evpPKey3, NULL, NULL, 0, 0, NULL));
			assertTrue (0 != PEM_write_bio_PUBKEY(bioPub2, evpPKey3));
			sizePriv2 = BIO_get_mem_data(bioPriv2, &pPrivData2);
			sizePub2 = BIO_get_mem_data(bioPub2, &pPubData2);

			assertTrue (sizePriv1 && (sizePriv1 == sizePriv2));
			assertTrue (0 == memcmp(pPrivData1, pPrivData2, sizePriv1));
			assertTrue (sizePub1 && (sizePub1 == sizePub2));
			assertTrue (0 == memcmp(pPubData1, pPubData2, sizePub1));

			BIO_free(bioPub2);
			BIO_free(bioPriv2);
			BIO_free(bioPub1);
			BIO_free(bioPriv1);
		}
		else
			std::cerr << "No elliptic curves found!" << std::endl;
	}
	catch (Poco::Exception& ex)
	{
		std::cerr << ex.displayText() << std::endl;
		throw;
	}
}


void EVPTest::testECEVPSaveLoadStream()
{
	try
	{
		std::string curveName = ECKey::getCurveName();
		if (!curveName.empty())
		{
			EVPPKey key(curveName);
			std::ostringstream strPub;
			std::ostringstream strPriv;
			key.save(&strPub, &strPriv, "testpwd");
			std::string pubKey = strPub.str();
			std::string privKey = strPriv.str();

			// now do the round trip
			std::istringstream iPub(pubKey);
			std::istringstream iPriv(privKey);
			EVPPKey key2(&iPub, &iPriv, "testpwd");

			std::ostringstream strPubE;
			std::ostringstream strPrivE;
			key2.save(&strPubE, &strPrivE, "testpwd");
			assertTrue (strPubE.str() == pubKey);
			/*TODO: figure out why EVP_PKEY_cmp() fails for identical public keys
			assertTrue (key == key2);
			assertTrue (!(key != key2));*/
			ECKey ecKeyNE(curveName);
			EVPPKey keyNE(&ecKeyNE);
			assertTrue (key != keyNE);
			assertTrue (!(key == keyNE));
			assertTrue (key2 != keyNE);
			assertTrue (!(key2 == keyNE));

			std::ostringstream strPub2;
			std::ostringstream strPriv2;
			key2.save(&strPub2, &strPriv2, "testpwd");
			assertTrue (strPub2.str() == pubKey);

			std::istringstream iPriv2(strPriv2.str());
			EVPPKey key3(0, &iPriv2,  "testpwd");
			std::ostringstream strPub3;
			key3.save(&strPub3);
			std::string pubFromPrivate = strPub3.str();
			assertTrue (pubFromPrivate == pubKey);
		}
		else
			std::cerr << "No elliptic curves found!" << std::endl;
	}
	catch (Poco::Exception& ex)
	{
		std::cerr << ex.displayText() << std::endl;
		throw;
	}
}


void EVPTest::testECEVPSaveLoadStreamNoPass()
{
	try
	{
		std::string curveName = ECKey::getCurveName();
		if (!curveName.empty())
		{
			EVPPKey key(curveName);
			std::ostringstream strPub;
			std::ostringstream strPriv;
			key.save(&strPub, &strPriv);
			std::string pubKey = strPub.str();
			std::string privKey = strPriv.str();

			// now do the round trip
			std::istringstream iPub(pubKey);
			std::istringstream iPriv(privKey);
			EVPPKey key2(&iPub, &iPriv);

			std::ostringstream strPubE;
			std::ostringstream strPrivE;
			key2.save(&strPubE, &strPrivE);
			assertTrue (strPubE.str() == pubKey);
			/*TODO: figure out why EVP_PKEY_cmp() fails for identical public keys
			assertTrue (key == key2);
			assertTrue (!(key != key2));*/
			ECKey ecKeyNE(curveName);
			EVPPKey keyNE(&ecKeyNE);
			assertTrue (key != keyNE);
			assertTrue (!(key == keyNE));
			assertTrue (key2 != keyNE);
			assertTrue (!(key2 == keyNE));

			std::ostringstream strPub2;
			std::ostringstream strPriv2;
			key2.save(&strPub2, &strPriv2);
			assertTrue (strPub2.str() == pubKey);
			assertTrue (strPriv2.str() == privKey);

			std::istringstream iPriv2(privKey);
			EVPPKey key3(0, &iPriv2);
			std::ostringstream strPub3;
			key3.save(&strPub3);
			std::string pubFromPrivate = strPub3.str();
			assertTrue (pubFromPrivate == pubKey);
		}
		else
			std::cerr << "No elliptic curves found!" << std::endl;
	}
	catch (Poco::Exception& ex)
	{
		std::cerr << ex.displayText() << std::endl;
		throw;
	}
}


void EVPTest::testECEVPSaveLoadFile()
{
	try
	{
		std::string curveName = ECKey::getCurveName();
		if (!curveName.empty())
		{
			EVPPKey key(curveName);
			TemporaryFile filePub;
			TemporaryFile filePriv;
			key.save(filePub.path(), filePriv.path(), "testpwd");
			std::ifstream ifPub(filePub.path());
			std::ifstream ifPriv(filePriv.path());
			std::string pubKey;
			std::string privKey;
			StreamCopier::copyToString(ifPub, pubKey);
			StreamCopier::copyToString(ifPriv, privKey);

			EVPPKey key2(filePub.path(), filePriv.path(), "testpwd");

			std::ostringstream strPubE;
			std::ostringstream strPrivE;
			key2.save(&strPubE, &strPrivE, "testpwd");
			assertTrue (strPubE.str() == pubKey);
			/*TODO: figure out why EVP_PKEY_cmp() fails for identical public keys
			assertTrue (key == key2);
			assertTrue (!(key != key2));*/
			ECKey ecKeyNE(curveName);
			EVPPKey keyNE(&ecKeyNE);
			assertTrue (key != keyNE);
			assertTrue (!(key == keyNE));
			assertTrue (key2 != keyNE);
			assertTrue (!(key2 == keyNE));

			std::ostringstream strPub2;
			std::ostringstream strPriv2;
			key2.save(&strPub2, &strPriv2, "testpwd");
			assertTrue (strPub2.str() == pubKey);

			EVPPKey key3("", filePriv.path(),  "testpwd");
			std::ostringstream strPub3;
			key3.save(&strPub3);
			std::string pubFromPrivate = strPub3.str();
			assertTrue (pubFromPrivate == pubKey);
		}
		else
			std::cerr << "No elliptic curves found!" << std::endl;
	}
	catch (Poco::Exception& ex)
	{
		std::cerr << ex.displayText() << std::endl;
		throw;
	}
}


void EVPTest::testECEVPSaveLoadFileNoPass()
{
	try
	{
		std::string curveName = ECKey::getCurveName();
		if (!curveName.empty())
		{
			EVPPKey key(curveName);
			TemporaryFile filePub;
			TemporaryFile filePriv;
			key.save(filePub.path(), filePriv.path());
			std::ifstream ifPub(filePub.path());
			std::ifstream ifPriv(filePriv.path());
			std::string pubKey;
			std::string privKey;
			StreamCopier::copyToString(ifPub, pubKey);
			StreamCopier::copyToString(ifPriv, privKey);

			EVPPKey key2(filePub.path(), filePriv.path());
			std::ostringstream strPub2;
			std::ostringstream strPriv2;
			key2.save(&strPub2, &strPriv2);
			assertTrue (strPub2.str() == pubKey);

			EVPPKey key3("", filePriv.path());
			std::ostringstream strPub3;
			key3.save(&strPub3);
			std::string pubFromPrivate = strPub3.str();
			assertTrue (pubFromPrivate == pubKey);
		}
		else
			std::cerr << "No elliptic curves found!" << std::endl;
	}
	catch (Poco::Exception& ex)
	{
		std::cerr << ex.displayText() << std::endl;
		throw;
	}
}


void EVPTest::setUp()
{
}


void EVPTest::tearDown()
{
}


CppUnit::Test* EVPTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("EVPTest");

	CppUnit_addTest(pSuite, EVPTest, testRSAEVPPKey);
	CppUnit_addTest(pSuite, EVPTest, testRSAEVPSaveLoadStream);
	CppUnit_addTest(pSuite, EVPTest, testRSAEVPSaveLoadStreamNoPass);
	CppUnit_addTest(pSuite, EVPTest, testECEVPPKey);
	CppUnit_addTest(pSuite, EVPTest, testECEVPSaveLoadStream);
	CppUnit_addTest(pSuite, EVPTest, testECEVPSaveLoadStreamNoPass);
	CppUnit_addTest(pSuite, EVPTest, testECEVPSaveLoadFile);
	CppUnit_addTest(pSuite, EVPTest, testECEVPSaveLoadFileNoPass);

	return pSuite;
}
