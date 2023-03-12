#ifndef MATH_ANIMATIONS_SERIALIZATION_HPP
#define MATH_ANIMATIONS_SERIALIZATION_HPP

// ------------------- Serialization helpers -------------------
#define SERIALIZE_NULLABLE_CSTRING(j, obj, prop) \
  j[#prop] = (obj)->prop

#define SERIALIZE_NULLABLE_U8_CSTRING(j, obj, prop) \
  j[#prop] = (char*)(obj)->prop

#define SERIALIZE_OBJECT(j, obj, prop) \
  (obj)->prop.serialize(j[#prop])

#define SERIALIZE_OBJECT_PTR(j, obj, prop) \
  (obj)->prop->serialize(j[#prop])

#define SERIALIZE_NON_NULL_PROP(j, obj, prop) \
  j[#prop] = (obj)->prop

#define SERIALIZE_ENUM(j, obj, prop, enumNamesArray) \
  j[#prop] = enumNamesArray[(size_t)(obj)->prop]

#define SERIALIZE_ID(j, obj, prop) \
  writeIdToJson(#prop, (obj)->prop, j)

#define SERIALIZE_ID_ARRAY(j, obj, prop) \
do { \
  j[#prop] = nlohmann::json::array(); \
  for (auto id : (obj)->prop) { \
  	  nlohmann::json jsonId{}; \
  	  convertIdToJson(id, jsonId); \
  	  j[#prop].emplace_back(jsonId); \
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
#define DESERIALIZE_NULLABLE_U8_CSTRING(obj, prop, j) \
do { \
  const std::string& str = j.contains(#prop) && !j[#prop].is_null() ? j[#prop] : "Undefined"; \
  (obj)->##prop##Length = (uint32)str.length(); \
  (obj)->prop = (uint8*)g_memory_allocate(sizeof(uint8) * ((obj)->##prop##Length + 1)); \
  g_memory_copyMem((obj)->prop, (void*)str.c_str(), sizeof(uint8) * ((obj)->##prop##Length + 1)); \
} while(false)

#define DESERIALIZE_PROP(obj, prop, j, defaultValue) \
  (obj)->prop = j.contains(#prop) && !j[#prop].is_null() ? j[#prop] : defaultValue;

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
    	AnimObjId id = convertJsonToId(j[#prop][i]); \
    	(obj)->prop.emplace_back(id); \
    } \
  } \
} while(false)

#define DESERIALIZE_ID_SET(obj, prop, j) \
do { \
  (obj)->prop = {}; \
  if (j.contains(#prop) && !j[#prop].is_null()) { \
    for (size_t i = 0; i < j[#prop].size(); i++) { \
    	AnimObjId id = convertJsonToId(j[#prop][i]); \
    	(obj)->prop.insert(id); \
    } \
  } \
} while(false)

#define DESERIALIZE_OBJECT(obj, prop, Type, version, j) \
  (obj)->prop = Type::deserialize(j[#prop], version)

#define _DESERIALIZE_VEC_CALL(obj, prop, j, vecType, numComponents) \
  CMath::deserialize##vecType##numComponents(j[#prop])

#define _DESERIALIZE_VEC(obj, prop, j, vecType, numComponents) \
  (obj)->prop = _DESERIALIZE_VEC_CALL(obj, prop, j, vecType, numComponents)

#define _DESERIALIZE_GLM_VEC(obj, prop, j, vecType, numComponents) \
  (obj)->prop = CMath::convert(_DESERIALIZE_VEC_CALL(obj, prop, j, vecType, numComponents))

// ------ My Vector types ------
#define DESERIALIZE_VEC2(obj, prop, j) _DESERIALIZE_VEC(obj, prop, j, Vec, 2)
#define DESERIALIZE_VEC3(obj, prop, j) _DESERIALIZE_VEC(obj, prop, j, Vec, 3)
#define DESERIALIZE_VEC4(obj, prop, j) _DESERIALIZE_VEC(obj, prop, j, Vec, 4)

#define DESERIALIZE_VEC2i(obj, prop, j) _DESERIALIZE_VEC(obj, prop, j, Vec, 2i)
#define DESERIALIZE_VEC3i(obj, prop, j) _DESERIALIZE_VEC(obj, prop, j, Vec, 3i)
#define DESERIALIZE_VEC4i(obj, prop, j) _DESERIALIZE_VEC(obj, prop, j, Vec, 4i)

#define DESERIALIZE_U8VEC4(obj, prop, j) _DESERIALIZE_VEC(obj, prop, j, U8Vec, 4)

// ------ GLM Vector types ------
#define DESERIALIZE_GLM_VEC2(obj, prop, j) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 2)
#define DESERIALIZE_GLM_VEC3(obj, prop, j) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 3)
#define DESERIALIZE_GLM_VEC4(obj, prop, j) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 4)

#define DESERIALIZE_GLM_VEC2i(obj, prop, j) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 2i)
#define DESERIALIZE_GLM_VEC3i(obj, prop, j) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 3i)
#define DESERIALIZE_GLM_VEC4i(obj, prop, j) _DESERIALIZE_GLM_VEC(obj, prop, j, Vec, 4i)

#define DESERIALIZE_GLM_U8VEC4(obj, prop, j) _DESERIALIZE_VEC(obj, prop, j, U8Vec, 4)

#endif