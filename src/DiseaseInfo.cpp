#include "BufferOperations.h"
#include "DiseaseInfo.h"


void DiseaseInfo::WriteData(std::shared_ptr<DiseaseInfo> dinfo, unsigned char* buffer, int offset)
{
	switch (version)
	{
	case 0x01000001:
		if (dinfo)
		{
			// write actual information
			Buffer::Write(valid_ver1, buffer, offset);
			// disease
			Buffer::Write(static_cast<uint32_t>(dinfo->disease), buffer, offset);
			// stage
			Buffer::Write(dinfo->stage, buffer, offset);
			// advPoints
			Buffer::Write(dinfo->advPoints, buffer, offset);
			// earliestAdvancement
			Buffer::Write(dinfo->earliestAdvancement, buffer, offset);
			// status
			Buffer::Write(static_cast<uint32_t>(dinfo->status), buffer, offset);
			// immuneUntil
			Buffer::Write(dinfo->immuneUntil, buffer, offset);
			// permanentModifiersPoints
			Buffer::Write(dinfo->permanentModifiersPoints, buffer, offset);
			// permanentModifiers
			Buffer::Write(dinfo->permanentModifiers, buffer, offset);
			return;
		}
		else
		{
			// empty info, write dummy
			Buffer::Write(invalid_ver1, buffer, offset);
			return;
		}
	}
}


std::shared_ptr<DiseaseInfo> DiseaseInfo::ReadData(unsigned char* buffer, int offset)
{
	std::shared_ptr<DiseaseInfo> ptr;
	uint32_t valid = Buffer::ReadUInt32(buffer, offset);
	{
		switch (valid)
		{
		case valid_ver1:
			{
				// valid total size 32
				ptr = std::make_shared<DiseaseInfo>();
				ptr->disease = static_cast<Diseases::Disease>(Buffer::ReadUInt32(buffer, offset));
				ptr->stage = Buffer::ReadInt32(buffer, offset);
				ptr->advPoints = Buffer::ReadFloat(buffer, offset);
				ptr->earliestAdvancement = Buffer::ReadFloat(buffer, offset);
				ptr->status = static_cast<DiseaseStatus>(Buffer::ReadUInt32(buffer, offset));
				ptr->immuneUntil = Buffer::ReadFloat(buffer, offset);
				ptr->permanentModifiersPoints = Buffer::ReadFloat(buffer, offset);
				ptr->permanentModifiers = Buffer::ReadUInt32(buffer, offset);
			}
			return ptr;
		case invalid_ver1:
			// invalid total size 4 bytes
			// just return nothing
			return ptr;
		}
	}
	return ptr;
}

int DiseaseInfo::GetDataSize(std::shared_ptr<DiseaseInfo> dinfo)
{
	if (dinfo) {
		switch (version) {
		case 0x01000001:
			// dinfovalid
			// 4
			// disease
			// 4
			// stage
			// 4
			// earliestAdvancement
			// 4
			// status
			// 4
			// immuneUntil
			// 4
			// permanentModifiersPoints
			// 4
			// permanentModifiers
			// 4
			return 32;
		}
	}
	else
	{
		switch (version)
		{
		case 0x01000001:
			// dinfoinvalid
			// 4
			return 4;
		}
	}
	return 4;
}
