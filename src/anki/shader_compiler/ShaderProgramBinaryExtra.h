// Copyright (C) 2009-2019, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/shader_compiler/Common.h>
#include <anki/gr/utils/Functions.h>
#include <anki/util/Serializer.h>

namespace anki
{

/// Serialize/deserialize ShaderVariableBlockInfo
template<typename TSerializer, typename TShaderVariableBlockInfo>
void serializeShaderVariableBlockInfo(TShaderVariableBlockInfo x, TSerializer& s)
{
	s.doValue("m_offset", offsetof(ShaderVariableBlockInfo, m_offset), x.m_offset);
	s.doValue("m_arraySize", offsetof(ShaderVariableBlockInfo, m_arraySize), x.m_arraySize);
	s.doValue("m_arrayStride", offsetof(ShaderVariableBlockInfo, m_arrayStride), x.m_arrayStride);
	s.doValue("m_matrixStride", offsetof(ShaderVariableBlockInfo, m_matrixStride), x.m_matrixStride);
}

/// Serialize ShaderVariableBlockInfo
template<>
class SerializeFunctor<ShaderVariableBlockInfo>
{
public:
	template<typename TSerializer>
	void operator()(const ShaderVariableBlockInfo& x, TSerializer& serializer)
	{
		serializeShaderVariableBlockInfo<TSerializer, const ShaderVariableBlockInfo&>(x, serializer);
	}
};

/// Deserialize ShaderVariableBlockInfo
template<>
class DeserializeFunctor<ShaderVariableBlockInfo>
{
public:
	template<typename TDeserializer>
	void operator()(ShaderVariableBlockInfo& x, TDeserializer& deserialize)
	{
		serializeShaderVariableBlockInfo<TDeserializer, ShaderVariableBlockInfo&>(x, deserialize);
	}
};

/// Serialize ActiveProgramInputVariableMask
template<typename TSerializer, typename T>
void serializeActiveProgramInputVariableMask(T x, TSerializer& s)
{
	s.doArray("bitset", 0, &x.getData()[0], x.getData().getSize());
}

/// Serialize ActiveProgramInputVariableMask
template<>
class SerializeFunctor<ActiveProgramInputVariableMask>
{
public:
	template<typename TSerializer>
	void operator()(const ActiveProgramInputVariableMask& x, TSerializer& serializer)
	{
		serializeActiveProgramInputVariableMask<TSerializer, const ActiveProgramInputVariableMask&>(x, serializer);
	}
};

/// Deserialize ActiveProgramInputVariableMask
template<>
class DeserializeFunctor<ActiveProgramInputVariableMask>
{
public:
	template<typename TDeserializer>
	void operator()(ActiveProgramInputVariableMask& x, TDeserializer& deserialize)
	{
		serializeShaderVariableBlockInfo<TDeserializer, ActiveProgramInputVariableMask&>(x, deserialize);
	}
};

} // end namespace anki
