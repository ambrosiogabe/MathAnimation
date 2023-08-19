/*
 * Base64 Decode
 * Polfosol
 *
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 */

 // Source code from Polfosol: https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c/13935718
 // Source code from Jouni Malinen: https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c


 // Encode/Decode functions originally modified by Juraj Ciberlin (jciberlin1@gmail.com) to be MISRA C 2012 compliant
 // Encode/Decode functions are modified again by Gabe Ambrosio (author of this repository)

#include "utils/base64.h"

namespace MathAnim
{
	namespace Base64
	{
		RawMemory encode(const uint8* data, size_t numBytes)
		{
			const uint8 base64Table[65] = {
				(uint8)'A', (uint8)'B', (uint8)'C', (uint8)'D',
				(uint8)'E', (uint8)'F', (uint8)'G', (uint8)'H',
				(uint8)'I', (uint8)'J', (uint8)'K', (uint8)'L',
				(uint8)'M', (uint8)'N', (uint8)'O', (uint8)'P',
				(uint8)'Q', (uint8)'R', (uint8)'S', (uint8)'T',
				(uint8)'U', (uint8)'V', (uint8)'W', (uint8)'X',
				(uint8)'Y', (uint8)'Z', (uint8)'a', (uint8)'b',
				(uint8)'c', (uint8)'d', (uint8)'e', (uint8)'f',
				(uint8)'g', (uint8)'h', (uint8)'i', (uint8)'j',
				(uint8)'k', (uint8)'l', (uint8)'m', (uint8)'n',
				(uint8)'o', (uint8)'p', (uint8)'q', (uint8)'r',
				(uint8)'s', (uint8)'t', (uint8)'u', (uint8)'v',
				(uint8)'w', (uint8)'x', (uint8)'y', (uint8)'z',
				(uint8)'0', (uint8)'1', (uint8)'2', (uint8)'3',
				(uint8)'4', (uint8)'5', (uint8)'6', (uint8)'7',
				(uint8)'8', (uint8)'9', (uint8)'+', (uint8_t)'/',
				(uint8)'\0'
			};

			const uint8* in = (const uint8*)data;
			size_t len = 4 * ((numBytes + 2) / 3);

			RawMemory res{};
			res.init(len + 1);

			size_t inPosition = 0;
			while ((numBytes - inPosition) >= 3U)
			{
				res.data[res.offset++] = base64Table[in[0] >> 2];
				res.data[res.offset++] = base64Table[((in[0] & 0x03U) << 4) | (in[1] >> 4)];
				res.data[res.offset++] = base64Table[((in[1] & 0x0FU) << 2) | (in[2] >> 6)];
				res.data[res.offset++] = base64Table[in[2] & 0x3FU];
				in += 3;
				inPosition += 3;
			}

			if (((numBytes - inPosition) != 0U))
			{
				res.data[res.offset++] = base64Table[in[0] >> 2];
				if ((numBytes - inPosition) == 1U)
				{
					res.data[res.offset++] = base64Table[(in[0] & 0x03U) << 4];
					res.data[res.offset++] = (uint8)'=';
				}
				else
				{
					res.data[res.offset++] = base64Table[((in[0] & 0x03U) << 4) | (in[1] >> 4)];
					res.data[res.offset++] = base64Table[(in[1] & 0x0FU) << 2];
				}

				res.data[res.offset++] = (uint8)'=';;
			}

			constexpr uint8 nullByte = '\0';
			res.write<uint8>(&nullByte);

			res.shrinkToFit();
			return res;
		}

		RawMemory decode(const char* b64String, size_t inLength)
		{
			const uint32_t base64Index[256] = {
				0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
				0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
				0U, 0U, 0U, 62U, 63U, 62U, 62U, 63U, 52U, 53U, 54U, 55U, 56U, 57U, 58U, 59U, 60U,
				61U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U,
				12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U, 20U, 21U, 22U, 23U, 24U, 25U, 0U, 0U, 0U,
				0U, 63U, 0U, 26U, 27U, 28U, 29U, 30U, 31U, 32U, 33U, 34U, 35U, 36U, 37U, 38U, 39U,
				40U, 41U, 42U, 43U, 44U, 45U, 46U, 47U, 48U, 49U, 50U, 51U, 0U, 0U, 0U, 0U, 0U, 0U,
				0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
				0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
				0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
				0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
				0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
				0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
				0U
			};

			const uint8* inDataUChar = (const uint8_t*)b64String;
			bool padBool = (inLength > 0) && (((inLength % 4) != 0) || (inDataUChar[inLength - 1] == (uint8_t)'='));
			uint32 padUint = 0;
			if (padBool)
			{
				padUint = 1;
			}

			const size_t length = (((inLength + 3) / 4) - padUint) * 4U;
			const size_t outLength = ((length / 4) * 3) + padUint;

			RawMemory res{};
			if (length == 0U)
			{
				return res;
			}

			res.init(outLength);

			for (size_t i = 0; i < length; i += 4)
			{
				uint32 n = (base64Index[inDataUChar[i]] << 18) 
					| (base64Index[inDataUChar[i + 1]] << 12) 
					| (base64Index[inDataUChar[i + 2]] << 6) 
					| (base64Index[inDataUChar[i + 3]]);

				const uint8 byte0 = (uint8_t)(n >> 16U);
				const uint8 byte1 = (uint8_t)((n >> 8U) & 0xFFU);
				const uint8 byte2 = (uint8_t)(n & 0xFFU);

				res.write<uint8>(&byte0);
				res.write<uint8>(&byte1);
				res.write<uint8>(&byte2);
			}

			if (padBool)
			{
				uint32 n = (base64Index[inDataUChar[length]] << 18) 
					| (base64Index[inDataUChar[length + 1]] << 12);

				const uint8 byte0 = (uint8_t)(n >> 16U);
				res.write<uint8>(&byte0);

				if ((inLength > (length + 2)) && (inDataUChar[length + 2] != (uint8_t)'='))
				{
					n |= base64Index[inDataUChar[length + 2]] << 6;
					uint8 byte1 = (uint8_t)((n >> 8U) & 0xFFU);
					res.write<uint8>(&byte1);
				}
			}

			res.shrinkToFit();
			return res;
		}
	}
}
