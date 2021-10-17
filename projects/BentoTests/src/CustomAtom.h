#pragma once
#include <Ap4.h>
#include <string>
#include <GLM/glm.hpp>

class CustomAtom : public AP4_Atom {
public:
	static CustomAtom* Create(AP4_Size size, AP4_ByteStream& stream) {
		return new CustomAtom(size, stream);
	}

	CustomAtom(const std::string& name, const glm::vec3& position = glm::vec3(0.0f));
	virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
	virtual AP4_Result WriteFields(AP4_ByteStream& stream);

protected:
	CustomAtom(AP4_Size size, AP4_ByteStream& stream);

	glm::vec3 position;
	std::string name;
};