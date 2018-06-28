/*--------------------------------------------------------------------------------*
 *                                                                                *
 * This material is trade secret owned by VoiceAge Corporation                    *
 * and is strictly confidential and shall remain as such.                         *
 *                                                                                *
 * Copyright ?1995-2001 VoiceAge Corporation. All Rights Reserved. No part of    *
 * this material may be reproduced, stored in a retrieval system, or transmitted, *
 * in any form or by any means, including, but not limited to, photocopying,      *
 *  electronic, mechanical, recording, or otherwise, without the prior written    *
 * permission of VoiceAge Corporation.                                            *
 *                                                                                *
 * This material is subject to continuous developments and improvements. All      *
 * warranties implied or expressed, including but not limited to implied          *
 * warranties of merchantability, or fitness for purpose, are excluded.           *
 *                                                                                *
 * ACELP and VoiceAge are registered trademark and trademark of VoiceAge          *
 * Corporation in Canada and / or other countries. Any unauthorized use is        *
 * strictly prohibited.                                                           *
 *                                                                                *
 *--------------------------------------------------------------------------------*
 *                                                                                *
 * VoiceAge Corporation                                                           *
 * 750, Chemin Lucerne                                                            *
 * Suite 250                                                                      *
 * Ville Mont-Royal (Quebec)                                                      *
 * Canada, H3R 2H6                                                                *
 *                                                                                *
 * Tel. (514) 737-4940, fax. (514) 908-2037                                       *
 *                                                                                *
 *--------------------------------------------------------------------------------*
 *
 *--------------------------------------------------------------------------------*
 *                            va_g729a.h					  *
 *                         ~~~~~~~~~~~~~~~~~~                                     *
 *--------------------------------------------------------------------------------*


/* va_g729a API functions prototypes and constants */
#ifndef  _WRAPPER_H_
#define  _WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif


int codecs_G729_open();
int codecs_G729_encode(short *pcm, unsigned char *G729Data, int lengthOfPcm);
int codecs_G729_decode(unsigned char *G729Data,  short *pcm, int lengthofG729);
int codecs_G729_close();

#ifdef __cplusplus
}
#endif


#endif  //end of _WRAPPER_H_