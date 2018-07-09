#include "Audio.h"

#include "Engine.h"

void Audio::play(Audio_Source* source) {
	Playing_Audio play_info;
	play_info.source = *source;
	
	//HACK!
	play_info.transform = engine->get_component_reference<Transform>(get_entity(source));
	
	ALint buffer, buffer_size;
	alGetSourcei(source->id, AL_BUFFER, &buffer);
	alGetBufferi(buffer, AL_SIZE, &buffer_size);
	float length = 2.0f; //hardcoded..... FIX.

	play_info.end_time = engine->time.engine_run_time + length;

	alSourcePlay(source->id);
	playing.push_back(play_info);
	playing_map[source->id] = playing.size() - 1;
}

void Audio::update(Time & time) {
	int count = playing.size();
	for (int i = count - 1; i >= 0; --i) {
		glm::vec3 pos = playing[i].transform.get()->position;
		alSource3f(playing[i].source.id, AL_POSITION, pos.x, pos.y, pos.z);
		//alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		
		if (time.engine_run_time > playing[i].end_time) {
			delete_playing(playing[i].source.id);
		}
	}
}

void Audio::delete_playing(ALuint id) {
	auto iter = playing_map.find(id);
	if (iter == playing_map.end())
		return;
	
	int index = iter->second;
	Playing_Audio &index_ref = playing[index];
	Playing_Audio &last_ref = playing[playing.size() - 1];
	playing_map[last_ref.source.id] = index; //We're swapping the indices, so we need to swap in the map as well.
	std::swap(index_ref, last_ref);
	playing.pop_back();
	playing_map.erase(id);
}

void Audio::delete_component(Entity entity) {
	ALuint id = get_component(entity)->id;
	if (id > 0) {
		alSourceStop(id);
		delete_playing(id);
		alDeleteSources((ALuint)1, &id);
	}
	
	System<Audio_Source>::delete_component(entity);
}

Audio::Audio(bool disable_output) : device(nullptr), context(nullptr) {
	channel_volumes.resize(SOUND_CHANNEL_COUNT);
	for (int i = 0; i < SOUND_CHANNEL_COUNT; ++i)
		channel_volumes[i] = 0.5f;
	
	if (!disable_output)
		initialize();
}

Audio::~Audio() {
	if (context) {
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
	}
	if (device) alcCloseDevice(device);
}

void Audio::initialize() {
	device = choose_device();

	//Create context
	context = alcCreateContext(device, nullptr);
	if (!alcMakeContextCurrent(context)) {
		Engine::fail("Unable to create sound context");
	}

	{	//Test...
		ALuint source;


		alGenSources((ALuint)1, &source);
		// check for errors

		alSourcef(source, AL_PITCH, 1.0f);
		// check for errors
		alSourcef(source, AL_GAIN, 1.0f);
		// check for errors
		alSource3f(source, AL_POSITION, 10.0f, 0.0f, 0.0f);
		// check for errors
		alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		// check for errors
		alSourcei(source, AL_LOOPING, AL_FALSE);
		// check for errros

		ALuint buffer = load_sound("blah");
		alSourcei(source, AL_BUFFER, buffer);

		alSourcePlay(source);
		alSourcei(source, AL_LOOPING, 1);
		//alGenBuffers((ALuint)1, &buffer);
		_sleep(200);
		set_volume(SOUND_CHANNEL_MASTER, 0.4f);
		_sleep(200);
		set_volume(SOUND_CHANNEL_MASTER, 0.3f);
		_sleep(200);
		set_volume(SOUND_CHANNEL_MASTER, 0.2f);
		_sleep(200);
		alSourceStop(source);
	}
}

void Audio::update() {
	ALfloat orientation[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };

	alListener3f(AL_POSITION, 0.0f, 0.0f, 1.0f);
	// check for errors
	alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	// check for errors
	alListenerfv(AL_ORIENTATION, orientation);
	// check for errors
}

ALuint Audio::load_sound(char * name) {
	//First, check if we already have the sound loaded.
	auto iter = buffers.find(name);
	if (iter != buffers.end())
		return iter->second;

	ALuint buffer;
	alGenBuffers((ALuint)1, &buffer);

	{	//Fill buffer with sine wave
		float frequency = 440.f;
		int seconds = 2;
		uint32_t sample_rate = 22050;
		uint32_t size = seconds * sample_rate;

		int16_t *samples = new int16_t[size];
		for (uint32_t i = 0; i < size; ++i) {
			samples[i] = 32760.f * sin((2.f*3.14159f*frequency) / (float)sample_rate * i);
		}

		alBufferData(buffer, AL_FORMAT_MONO16, samples, size, sample_rate);
	}

	buffers[name] = buffer;
	return buffer;
}

void Audio::unload_sound(char * name) {
	//We might have to count usage here..
}

void Audio::set_volume(Sound_Channel channel, float volume) {
	channel_volumes[channel] = volume;
	if (channel == SOUND_CHANNEL_MASTER) {
		alListenerf(AL_GAIN, volume);
	}
}

inline float Audio::get_volume(Sound_Channel channel) {
	return channel_volumes[channel];
}

ALuint Audio::create_audio_source(Sound_Channel channel, bool loop) {
	return create_audio_source(channel, loop, "blah");
}

ALuint Audio::create_audio_source(Sound_Channel channel, bool loop, char* sound_name) {
	ALuint source;
	alGenSources((ALuint)1, &source);
	//alSourcef(source, AL_PITCH, 1.0f);

	if (sound_name) {
		ALuint buffer = load_sound(sound_name);
		alSourcei(source, AL_BUFFER, buffer);
	}

	alSourcef(source, AL_GAIN, channel_volumes[channel]);
	alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);

	return source;
}

ALCdevice* Audio::choose_device() {
	ALboolean can_enumerate_devices;

	can_enumerate_devices = alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT");
	if (can_enumerate_devices == AL_FALSE) {
		ALCdevice* dev = alcOpenDevice(nullptr);
		if (!dev) {
			Engine::fail("Unable to find sound device");
		}
		return dev;
	} else {
		const char* device_string = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
		int len = 0;
		std::vector<const char*> devices;
		while ((len = strlen(device_string)) > 0) {
			devices.push_back(device_string);
			device_string += len + 1;
		}

		if (devices.size() == 0) {
			Engine::fail("Unable to find any sound device");
		}

		ALCdevice* dev = alcOpenDevice(devices[0]);
		if (!dev) {
			Engine::fail("Unable to open sound device");
		}
		return dev;
	}

	return nullptr;
}