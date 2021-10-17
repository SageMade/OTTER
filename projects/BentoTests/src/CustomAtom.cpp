#include "CustomAtom.h"
#include <sstream>

const AP4_Atom::Type HEADER_TYPE = AP4_ATOM_TYPE('c', 'u', 's', 't');

CustomAtom::CustomAtom(const std::string& name, const glm::vec3& position)
	: AP4_Atom(HEADER_TYPE, AP4_ATOM_HEADER_SIZE + sizeof(float) * 3 + name.length() + sizeof(uint16_t))
{
}

AP4_Result CustomAtom::InspectFields(AP4_AtomInspector& inspector)
{
	std::stringstream stream;
	stream << position.x << "," << position.y << "," << position.z;
	inspector.AddField("name", name.c_str());
	inspector.AddField("position", stream.str().c_str());
	return AP4_SUCCESS;
}

AP4_Result CustomAtom::WriteFields(AP4_ByteStream& stream)
{
	stream.Write(&position.x, sizeof(float) * 3);
	stream.WriteUI16(static_cast<uint16_t>(name.length()));
	stream.WriteString(name.c_str());
	return AP4_SUCCESS;
}

CustomAtom::CustomAtom(AP4_Size size, AP4_ByteStream& stream)
	: AP4_Atom(HEADER_TYPE, AP4_ATOM_HEADER_SIZE)
{
	stream.Read(&position.x, sizeof(float) * 3);
	uint16_t nameLen = 0;
	stream.Read(&nameLen, sizeof(uint16_t));
	char* nameCstr = new char[nameLen];
	stream.Read(nameCstr, nameLen);
	name = std::string(nameCstr, nameLen);
	SetSize(static_cast<uint32_t>(AP4_ATOM_HEADER_SIZE + sizeof(float) * 3 + name.length() + sizeof(uint16_t)));
}
