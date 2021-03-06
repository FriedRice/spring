/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

#include "AssimpPCH.h"
#include "M3Importer.h"

namespace Assimp {
namespace M3 {

static const std::string M3Extension = "m3";

// ------------------------------------------------------------------------------------------------
//	Constructor.
M3Importer::M3Importer() :
	m_pHead( NULL ),
	m_pRefs( NULL )
{

}

// ------------------------------------------------------------------------------------------------
//	Destructor.
M3Importer::~M3Importer()
{
	m_pHead = NULL;
	m_pRefs = NULL;
}

// ------------------------------------------------------------------------------------------------
//	Check for readable file format.
bool M3Importer::CanRead( const std::string &rFile, IOSystem* /*pIOHandler*/, bool checkSig ) const
{
	if ( !checkSig )
		return SimpleExtensionCheck( rFile, M3Extension.c_str() );

	return true;
}

// ------------------------------------------------------------------------------------------------
void M3Importer::GetExtensionList(std::set<std::string>& extensions)
{

}

// ------------------------------------------------------------------------------------------------
void M3Importer::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler )
{

}

// ------------------------------------------------------------------------------------------------

} // Namespace M3
} // Namespace Assimp
