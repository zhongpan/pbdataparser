#include "google/protobuf/io/coded_stream.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
//#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/compiler/importer.h>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>

using namespace google::protobuf;
using namespace google::protobuf::compiler;

class PrinterErrorCollector : public MultiFileErrorCollector
{
public:
	virtual void AddError(const string& filename, int line, int column,
		const string& message)
	{
		printf("!!!%s: %s(%d:%d)\n", message.c_str(), filename.c_str(), line, column);
	}
};

class PbDataParser
{

public:
	PbDataParser()
	{
	}

	bool Parse(const std::string &filePath, const std::string &protoFilePath, const std::string &messageTypeName)
	{
		DiskSourceTree sourceTree;
		std::filesystem::path proto(protoFilePath);
		sourceTree.MapPath("", proto.parent_path().string());
		PrinterErrorCollector error_collector;
		Importer importer(&sourceTree, &error_collector);
		if (!importer.Import(proto.filename().string()))
		{
			printf("!!!import proto file failed : %s", protoFilePath.c_str());
			return false;
		}
		std::ifstream ifs(filePath.c_str(), std::ios::in | std::ios::binary);
		if (!ifs.good())
		{
			printf("!!!open file failed : %s", filePath.c_str());
			return false;
		}
		io::IstreamInputStream zero_copy_input(&ifs);
		io::CodedInputStream decoder(&zero_copy_input);
		const Descriptor *descriptor = importer.pool()->FindMessageTypeByName(messageTypeName);
		if (!descriptor)
		{
			printf("!!!message type not exist : %s", messageTypeName.c_str());
			return false;
		}

		if (DoParse(&decoder, descriptor, 0) && ifs.eof())
		{
			std::cout << "parse success!" << std::endl;
		}
		else
		{
			const void *left = NULL;
			int size = 0;
			decoder.GetDirectBufferPointerInline(&left, &size);
			printf("!!!parse failed, left bytes(loc:%d,size:%d):\n", decoder.CurrentPosition(), size);
			const void *end = (unsigned char *)left + size;
			std::cout << ToHexStr(left, end, 16, 64) << std::endl;
		}

	}

private:
	std::string ToHexStr(const void *start, const void *end, int linelength = -1, size_t maxsize = 0)
	{
		size_t size = (unsigned char *)end - (unsigned char *)start;
		std::string hexStr;
		unsigned char * p = (unsigned char *)start;
		unsigned char * print_end = (unsigned char *)end;
		if (maxsize > 0 && maxsize < size)
		{
			print_end = p + maxsize;
		}
		char buf[3] = { 0 };
		for (int i = 1; p < print_end; p++, i++)
		{
			sprintf(buf, "%02x", *p);
			hexStr += buf;
			if (linelength > 0 && i % linelength == 0)
			{
				hexStr += "\n";
			}
		}
		if (maxsize > 0 && maxsize < size)
		{
			hexStr += "...";
		}
		return hexStr;
	}

	bool DoParse(io::CodedInputStream *input, const Descriptor *descriptor, int layer)
	{
		while (true)
		{
			uint32 tag = 0;
			const void *tagstart = NULL;
			int size = 0;
			input->GetDirectBufferPointerInline(&tagstart, &size);
			int tagstartloc = input->CurrentPosition();
			tag = input->ReadTag();
			if (0 == tag)
			{
				break;
			}
			int fieldNo = internal::WireFormatLite::GetTagFieldNumber(tag);
			internal::WireFormatLite::WireType type = internal::WireFormatLite::GetTagWireType(tag);
			const FieldDescriptor *fieldDescriptor = descriptor->FindFieldByNumber(fieldNo);
			if (!fieldDescriptor)
			{
				printf("!!!field not exist, maybe messagetype wrong : fieldNo=%d, wire_type=%d\n", fieldNo, type);
				return false;
			}
			FieldDescriptor::Type fieldType = fieldDescriptor->type();
			const void *tagend = NULL;
			input->GetDirectBufferPointerInline(&tagend, &size);
			int tagendloc = input->CurrentPosition();
			for (int i = 0; i < layer; i++) std::cout << "  ";
			printf("%s = loc(%d),tag[0x%s(%d,%d)],", fieldDescriptor->name().c_str(), tagstartloc,
				ToHexStr((unsigned char *)tagend - (tagendloc - tagstartloc), tagend).c_str(), fieldNo, type);
			if (FieldDescriptor::TYPE_INT32 == fieldType)
			{
				int32 v;
				internal::WireFormatLite::ReadPrimitive<int32, internal::WireFormatLite::TYPE_INT32>(input, &v);
				const void *fieldend = NULL;
				input->GetDirectBufferPointerInline(&fieldend, &size);
				int fieldendloc = input->CurrentPosition();
				printf("value[0x%s(%d)]\n", ToHexStr((unsigned char *)fieldend - (fieldendloc - tagendloc), fieldend).c_str(), v);
			}
			else if (FieldDescriptor::TYPE_STRING == fieldType)
			{
				std::string v;
				internal::WireFormatLite::ReadBytes(input, &v);
				const void *fieldend = NULL;
				input->GetDirectBufferPointerInline(&fieldend, &size);
				int fieldendloc = input->CurrentPosition();
				printf("len&value[0x%s(%s)]\n", ToHexStr((unsigned char *)fieldend - (fieldendloc - tagendloc), fieldend).c_str(), v.c_str());
			}
			else if (FieldDescriptor::TYPE_BYTES == fieldType)
			{
				std::string v;
				internal::WireFormatLite::ReadBytes(input, &v);
				const void *fieldend = NULL;
				input->GetDirectBufferPointerInline(&fieldend, &size);
				int fieldendloc = input->CurrentPosition();
				printf("len&value[0x%s]\n", ToHexStr((unsigned char *)fieldend - (fieldendloc - tagendloc), fieldend).c_str());
			}
			else if (FieldDescriptor::TYPE_MESSAGE == fieldType)
			{
				uint32 length;
				if (!input->ReadVarint32(&length))
				{
					return false;
				}
				const void *lenend = NULL;
				input->GetDirectBufferPointerInline(&lenend, &size);
				int lenloc = input->CurrentPosition();
				printf("len[0x%s(%d)]\n", ToHexStr((unsigned char *)lenend - (lenloc - tagendloc), lenend).c_str(), length);
				if (!input->IncrementRecursionDepth())
				{
					return false;
				}
				io::CodedInputStream::Limit limit = input->PushLimit(length);
				if (!DoParse(input, fieldDescriptor->message_type(), layer + 1))
				{
					return false;
				}
				if (!input->ConsumedEntireMessage())
				{
					return false;
				}
				input->PopLimit(limit);
				input->DecrementRecursionDepth();
			}
			else
			{
				printf("!!!type not implemented : fieldname=%s, wire_type=%d, fieldType=%d\n",
					fieldDescriptor->full_name().c_str(), type, fieldType);
				return false;
			}
			//UnknownFieldSet unknownFields;
			//internal::WireFormat::SkipField(input, tag, &unknownFields);
		}
		return input->ConsumedEntireMessage();
	}

private:

};


int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		printf("usage: pbdataparser <pbdatafile> <protofile> <messagetype>");
		return -1;
	}
	PbDataParser parser;
	if (!parser.Parse(argv[1], argv[2], argv[3]))
	{
		return -1;
	}
	return 0;
}

