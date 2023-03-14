#ifndef MATH_ANIMATIONS_SERIALIZATION_HPP
#define MATH_ANIMATIONS_SERIALIZATION_HPP
#include "core.h"

#include <nlohmann/json.hpp>

inline void writeIdToJson(const char* propertyName, AnimObjId id, nlohmann::json& j)
{
	if (MathAnim::isNull(id))
	{
		j[propertyName] = nullptr;
	}
	else
	{
		j[propertyName] = id;
	}
}

inline void convertIdToJson(AnimObjId id, nlohmann::json& j)
{
	if (MathAnim::isNull(id))
	{
		j = nullptr;
	}
	else
	{
		j = id;
	}
}

[[nodiscard]]
inline AnimId readIdFromJson(const nlohmann::json& j, const char* propertyName)
{
	if (j.contains(propertyName))
	{
		if (!j[propertyName].is_null())
		{
			return j[propertyName];
		}
	}

	return MathAnim::NULL_ANIM;
}

[[nodiscard]]
inline AnimId convertJsonToId(const nlohmann::json& j)
{
	if (j.is_null())
	{
		return MathAnim::NULL_ANIM;
	}

	return j;
}

// ------------------- Serialization helpers -------------------
#define SERIALIZE_NULLABLE_CSTRING(j, obj, prop, defaultValue) \
  j[#prop] = (obj)->prop == nullptr ? defaultValue : (obj)->prop;

#define SERIALIZE_NULLABLE_U8_CSTRING(j, obj, prop, defaultValue) \
  j[#prop] = (obj)->prop == nullptr ? defaultValue : (char*)(obj)->prop

#define SERIALIZE_OBJECT(j, obj, prop) \
  (obj)->prop.serialize(j[#prop])

#define SERIALIZE_OBJECT_PTR(j, obj, prop) \
  (obj)->prop->serialize(j[#prop])

#define SERIALIZE_NON_NULL_PROP(j, obj, prop) \
  j[#prop] = (obj)->prop

#define SERIALIZE_VALUE_INLINE(j, prop, value) \
  j[#prop] = value

#define SERIALIZE_NON_NULL_PROP_BY_VALUE(j, obj, prop, value) \
  if (obj != nullptr) { (obj)->prop; } /* NOP; This will catch any typos at compile time */ \
  j[#prop] = value;

#define SERIALIZE_ENUM(j, obj, prop, enumNamesArray) \
  j[#prop] = enumNamesArray[(size_t)(obj)->prop]

#define SERIALIZE_ID(j, obj, prop) \
  writeIdToJson(#prop, (obj)->prop, j)

#define SERIALIZE_ID_ARRAY(j, obj, prop) \
do { \
  j[#prop] = nlohmann::json::array(); \
  for (auto _internalId : (obj)->prop) { \
  	  nlohmann::json _internalJsonId{}; \
  	  convertIdToJson(_internalId, _internalJsonId); \
  	  j[#prop].emplace_back(_internalJsonId); \
  } \
} while(false)

#define SERIALIZE_SIMPLE_ARRAY(j, obj, prop) \
do { \
  j[#prop] = nlohmann::json::array(); \
  for (const auto& _data : (obj)->prop) { \
  	  j[#prop].emplace_back(_data); \
  } \
} while(false)

// ------ My Vector Types ------
#define SERIALIZE_VEC(j, obj, prop) \
  CMath::serialize(j, #prop, (obj)->prop)

// ------ GLM Vector types ------
#define SERIALIZE_GLM_VEC4(j, obj, prop) \
  CMath::serialize(j, #prop, Vec4{(obj)->prop.x, (obj)->prop.y, (obj)->prop.z, (obj)->prop.w})

#define SERIALIZE_GLM_VEC3(j, obj, prop) \
  CMath::serialize(j, #prop, Vec3{(obj)->prop.x, (obj)->prop.y, (obj)->prop.z})

#define SERIALIZE_GLM_VEC2(j, obj, prop) \
  CMath::serialize(j, #prop, Vec2{(obj)->prop.x, (obj)->prop.y})

#define SERIALIZE_GLM_U8VEC4(j, obj, prop) \
  CMath::serialize(j, #prop, Vec4{(obj)->prop.r, (obj)->prop.g, (obj)->prop.b, (obj)->prop.a})

// ------------------- Deserialization helpers -------------------
#define DESERIALIZE_NULLABLE_CSTRING(obj, prop, j) \
do { \
  const std::string& str = j.contains(#prop) && !j[#prop].is_null() ? j[#prop] : "Undefined"; \
  (obj)->##prop##Length = (uint32)str.length(); \
  (obj)->prop = (char*)g_memory_allocate(sizeof(char) * ((obj)->##prop##Length + 1)); \
  g_memory_copyMem((obj)->prop, (void*)str.c_str(), sizeof(char) * ((obj)->##prop##Length + 1)); \
} while(false)

#define DESERIALIZE_NULLABLE_U8_CSTRING(obj, prop, j) \
do { \
  const std::string& str = j.contains(#prop) && !j[#prop].is_null() ? j[#prop] : "Undefined"; \
  (obj)->##prop##Length = (uint32)str.length(); \
  (obj)->prop = (uint8*)g_memory_allocate(sizeof(uint8) * ((obj)->##prop##Length + 1)); \
  g_memory_copyMem((obj)->prop, (void*)str.c_str(), sizeof(uint8) * ((obj)->##prop##Length + 1)); \
} while(false)

#define DESERIALIZE_PROP(obj, prop, j, defaultValue) \
  (obj)->prop = j.contains(#prop) && !j[#prop].is_null() ? j[#prop] : defaultValue;

#define DESERIALIZE_VALUE_INLINE(j, prop, defaultValue) \
  j.contains(#prop) && !j[#prop].is_null() ? j[#prop] : defaultValue

#define DESERIALIZE_PROP_INLINE(obj, prop, j, defaultValue) \
  j.contains(#prop) && !j[#prop].is_null() ? j[#prop] : defaultValue; \
  do { if (obj != nullptr) { (obj)->prop; } } while(false) /* NOP; This is used to check for typos at compile time */ 

#define DESERIALIZE_ENUM(obj, prop, enumNamesArray, enumType, j) \
do { \
  const std::string& enumTypeStr = j.contains(#prop) && !j[#prop].is_null() ? j[#prop] : "Undefined"; \
  (obj)->prop = findMatchingEnum<enumType, (size_t)enumType::Length>(enumNamesArray, enumTypeStr); \
} while(false)

#define DESERIALIZE_ID(obj, prop, j) \
  (obj)->prop = readIdFromJson(j, #prop);

#define DESERIALIZE_ID_ARRAY(obj, prop, j) \
do { \
  (obj)->prop = {}; \
  if (j.contains(#prop) && !j[#prop].is_null()) { \
    for (size_t i = 0; i < j[#prop].size(); i++) { \
    	AnimObjId _internalId = convertJsonToId(j[#prop][i]); \
    	(obj)->prop.emplace_back(_internalId); \
    } \
  } \
} while(false)

#define DESERIALIZE_SIMPLE_ARRAY(obj, prop, j) \
do { \
  (obj)->prop = {}; \
  if (j.contains(#prop) && !j[#prop].is_null()) { \
    for (size_t i = 0; i < j[#prop].size(); i++) { \
    	(obj)->prop.emplace_back(j[#prop][i]); \
    } \
  } \
} while(false)

#define DESERIALIZE_ID_SET(obj, prop, j) \
do { \
  (obj)->prop = {}; \
  if (j.contains(#prop) && !j[#prop].is_null()) { \
    for (size_t i = 0; i < j[#prop].size(); i++) { \
    	AnimObjId _internalId = convertJsonToId(j[#prop][i]); \
    	(obj)->prop.insert(_internalId); \
    } \
  } \
} while(false)

#define DESERIALIZE_OBJECT(obj, prop, Type, version, j) \
do { \
  if (j.contains(#prop) && !j[#prop].is_null()) { \
    (obj)->prop = Type::deserialize(j[#prop], version); \
  } \
} while (false)

#define _DESERIALIZE_VEC_CALL(obj, prop, j, vecType, numComponents, defaultValue) \
  CMath::deserialize##vecType##numComponents(j.contains(#prop) ? j[#prop] : nlohmann::json(), defaultValue)

#define _DESERIALIZE_VEC(obj, prop, j, vecType, numComponents, defaultValue) \
  (obj)->prop = _DESERIALIZE_VEC_CALL(obj, prop, j, vecType, numComponents, defaultValue)

#define _DESERIALIZE_GLM_VEC(obj, prop, j, vecType, numComponents, defaultValue) \
  (obj)->prop = CMath::convert(_DESERIALIZE_VEC_CALL(obj, prop, j, vecType, numComponents, defaultValue))

// ------ My Vector types ------
#define DESERIALIZE_VEC2(obj, prop, j, defaultValue) _DESERIALIZE_VEC(obj, prop, j, Vec, 2, defaultValue)
#define DESERIALIZE_VEC3(obj, prop, j, defaultValue) _DESERIALIZE_VEC(obj, prop, j, Vec, 3, defaultValue)
#define DESERIALIZE_VEC4(obj, prop, j, defaultValue) _DESERIALIZE_VEC(obj, prop, j, Vec, 4, defaultValue)

#define DESERIALIZE_VEC2i(obj, prop, j, defaultValue) _DESERIALIZE_VEC(obj, prop, j, Vec, 2i, defaultValue)
#define DESERIALIZE_VEC3i(obj, prop, j, defaultValue) _DESERIALIZE_VEC(obj, prop, j, Vec, 3i, defaultValue)
#define DESERIALIZE_VEC4i(obj, prop, j, defaultValue) _DESERIALIZE_VEC(obj, prop, j, Vec, 4i, defaultValue)

#define DESERIALIZE_U8VEC4(obj, prop, j, defaultValue) _DESERIALIZE_VEC(obj, prop, j, U8Vec, 4, defaultValue)

// ------ GLM Vector types ------
#define DESERIALIZE_GLM_VEC2(obj, prop, j, defaultValue) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 2, defaultValue)
#define DESERIALIZE_GLM_VEC3(obj, prop, j, defaultValue) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 3, defaultValue)
#define DESERIALIZE_GLM_VEC4(obj, prop, j, defaultValue) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 4, defaultValue)

#define DESERIALIZE_GLM_VEC2i(obj, prop, j, defaultValue) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 2i, defaultValue)
#define DESERIALIZE_GLM_VEC3i(obj, prop, j, defaultValue) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 3i, defaultValue)
#define DESERIALIZE_GLM_VEC4i(obj, prop, j, defaultValue) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 4i, defaultValue)

#define DESERIALIZE_GLM_U8VEC4(obj, prop, j, defaultValue) _DESERIALIZE_VEC(obj, prop, j, U8Vec, 4, defaultValue)

#endif