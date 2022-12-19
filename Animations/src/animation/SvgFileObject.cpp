#include "animation/SvgFileObject.h"
#include "animation/AnimationManager.h"
#include "svg/Svg.h"
#include "svg/SvgParser.h"
#include "editor/SceneHierarchyPanel.h"

namespace MathAnim
{
	// Internal Functions
	static SvgFileObject deserializeSvgFileObjectV1(RawMemory& memory);

	void SvgFileObject::init(AnimationManagerData* am, AnimObjId parentId)
	{
		if (!svgGroup)
		{
			return;
		}

		// Generate children objects and place them accordingly
		// Each child represents a group sub-object

		for (int i = 0; i < svgGroup->numObjects; i++)
		{
			const SvgObject& obj = svgGroup->objects[i];
			const Vec2& offset = svgGroup->objectOffsets[i];

			// Add this sub-object as a child
			AnimObject childObj = AnimObject::createDefaultFromParent(am, AnimObjectTypeV1::SvgObject, parentId, true);
			childObj.parentId = parentId;

			childObj._positionStart = Vec3{ offset.x, offset.y, 0.0f };
			childObj.isGenerated = true;
			// Copy the sub-object as the svg object here
			childObj._svgObjectStart = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			childObj.svgObject = (SvgObject*)g_memory_allocate(sizeof(SvgObject));
			*(childObj._svgObjectStart) = Svg::createDefault();
			*(childObj.svgObject) = Svg::createDefault();
			Svg::copy(childObj._svgObjectStart, &obj);
			Svg::copy(childObj.svgObject, &obj);
			childObj._fillColorStart = glm::u8vec4(
				(uint8)(obj.fillColor.r * 255.0f),
				(uint8)(obj.fillColor.g * 255.0f),
				(uint8)(obj.fillColor.b * 255.0f),
				(uint8)(obj.fillColor.a * 255.0f)
			);
			childObj.fillColor = childObj._fillColorStart;

			const char childName[] = "Generated Child";
			childObj.nameLength = sizeof(childName);
			childObj.name = (uint8*)g_memory_realloc(childObj.name, sizeof(uint8) * (childObj.nameLength + 1));
			g_memory_copyMem(childObj.name, (void*)childName, sizeof(childName) / sizeof(char));
			childObj.name[childObj.nameLength] = '\0';

			AnimationManager::addAnimObject(am, childObj);
			// TODO: Ugly what do I do???
			SceneHierarchyPanel::addNewAnimObject(childObj);
		}
	}

	void SvgFileObject::reInit(AnimationManagerData* am, AnimObject* obj)
	{
		// First remove all generated children, which were generated as a result
		// of this object (presumably)
		// NOTE: This is direct descendants, no recursive children here
		for (int i = 0; i < obj->generatedChildrenIds.size(); i++)
		{
			AnimObject* child = AnimationManager::getMutableObject(am, obj->generatedChildrenIds[i]);
			if (child)
			{
				SceneHierarchyPanel::deleteAnimObject(*child);
				AnimationManager::removeAnimObject(am, obj->generatedChildrenIds[i]);
			}
		}
		obj->generatedChildrenIds.clear();

		// Next init again which should regenerate the children
		init(am, obj->id);
	}

	bool SvgFileObject::setFilepath(const std::string& newFilepath)
	{
		free();

		filepath = (char*)g_memory_allocate(sizeof(char) * (newFilepath.length() + 1));
		filepathLength = (uint32)newFilepath.length();

		g_memory_copyMem(filepath, (void*)newFilepath.c_str(), sizeof(char) * newFilepath.length());
		filepath[filepathLength] = '\0';

		// Try to parse the new SVG and reset to the old one if it fails
		if (filepath)
		{
			svgGroup = SvgParser::parseSvgDoc(filepath);
		}

		return svgGroup != nullptr;
	}

	bool SvgFileObject::setFilepath(const char* newFilepath)
	{
		return setFilepath(std::string(newFilepath));
	}

	void SvgFileObject::serialize(RawMemory& memory) const
	{
		// filepathLength       -> u32
		// filepath             -> u8[textLength]
		memory.write<uint32>(&filepathLength);
		if (filepathLength)
		{
			memory.writeDangerous((const uint8*)filepath, sizeof(uint8) * (filepathLength + 1));
		}
	}

	void SvgFileObject::free()
	{
		if (svgGroup)
		{
			svgGroup->free();
			g_memory_free(svgGroup);
			svgGroup = nullptr;
		}

		if (filepath)
		{
			g_memory_free(filepath);
			filepath = nullptr;
			filepathLength = 0;
		}
	}

	SvgFileObject SvgFileObject::deserialize(RawMemory& memory, uint32 version)
	{
		if (version == 1)
		{
			return deserializeSvgFileObjectV1(memory);
		}

		g_logger_error("Invalid version '%d' while deserializing text object.", version);
		SvgFileObject res;
		g_memory_zeroMem(&res, sizeof(SvgFileObject));
		return res;
	}

	SvgFileObject SvgFileObject::createDefault()
	{
		SvgFileObject res;
		res.filepath = nullptr;
		res.filepathLength = 0;
		res.svgGroup = nullptr;

		return res;
	}

	// -------------------- Internal Functions --------------------
	static SvgFileObject deserializeSvgFileObjectV1(RawMemory& memory)
	{
		SvgFileObject res = {};

		// filepathLength       -> u32
		// filepath             -> u8[textLength]

		memory.read<uint32>(&res.filepathLength);
		if (res.filepathLength > 0)
		{
			res.filepath = (char*)g_memory_allocate(sizeof(char) * (res.filepathLength + 1));
			memory.readDangerous((uint8*)res.filepath, res.filepathLength * sizeof(uint8));
			memory.read<char>(&res.filepath[res.filepathLength]);
		}
		else
		{
			res.filepath = nullptr;
		}

		res.svgGroup = nullptr;

		return res;
	}
}