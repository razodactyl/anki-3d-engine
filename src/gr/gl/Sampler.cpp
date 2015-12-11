// Copyright (C) 2009-2015, Panagiotis Christopoulos Charitos.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/gr/Sampler.h>
#include <anki/gr/gl/SamplerImpl.h>
#include <anki/gr/gl/CommandBufferImpl.h>

namespace anki {

//==============================================================================
Sampler::Sampler(GrManager* manager)
	: GrObject(manager)
{}

//==============================================================================
Sampler::~Sampler()
{}

//==============================================================================
class CreateSamplerCommand: public GlCommand
{
public:
	SamplerPtr m_sampler;
	SamplerInitializer m_init;

	CreateSamplerCommand(Sampler* sampler, const SamplerInitializer& init)
        : m_sampler(sampler)
		, m_init(init)
	{}

	Error operator()(GlState&)
	{
        SamplerImpl& impl = m_sampler->getImplementation();

		impl.create(m_init);

        GlObject::State oldState =
			impl.setStateAtomically(GlObject::State::CREATED);

		(void)oldState;
		ANKI_ASSERT(oldState == GlObject::State::TO_BE_CREATED);

		return ErrorCode::NONE;
	}
};

void Sampler::create(const SamplerInitializer& init)
{
	m_impl.reset(getAllocator().newInstance<SamplerImpl>(&getManager()));

	CommandBufferPtr cmdb = getManager().newInstance<CommandBuffer>();

	cmdb->getImplementation().pushBackNewCommand<CreateSamplerCommand>(
		this, init);
	cmdb->flush();
}

} // end namespace anki