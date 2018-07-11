#pragma once

#include "System.h"
#include "Transform_System.h"

#include <al.h>
#include <alc.h>
#include <unordered_map>

//enum Audio_Flags : uint16_t {
//	AUDIO_LOOP = 1,
//	AUDIO_PLAY_ON_SPAWN = 2,
//	AUDIO_
//
//};

enum Sound_Channel {
	SOUND_CHANNEL_MASTER,
	SOUND_CHANNEL_MUSIC,
	SOUND_CHANNEL_EFFECTS,
	SOUND_CHANNEL_AMBIENCE,
	SOUND_CHANNEL_UI,

	SOUND_CHANNEL_COUNT
};

struct Audio_Source {
	ALuint id = 0;
};

struct Playing_Audio {
	ALuint id;
	Entity entity;
	float end_time;
};

class Audio : public System<Audio_Source> {
protected:
	ALCdevice *device;
	ALCcontext *context;
	std::unordered_map<char*, ALuint> buffers;
	std::vector<float> channel_volumes;
	Entity listener;

	std::vector<Playing_Audio> playing;
	std::unordered_map<ALuint, int> playing_map;
	
	void initialize();
	ALCdevice* choose_device();
	void delete_playing(ALuint id);
public:
	Audio(bool server);
	~Audio();

	void set_listener_entity(Entity entity) { listener = entity; }
	ALuint load_sound(char* name);
	void unload_sound(char* name);
	void set_volume(Sound_Channel channel, float volume);
	float get_volume(Sound_Channel channel);
	ALuint create_audio_source(Sound_Channel channel, bool loop);
	ALuint create_audio_source(Sound_Channel channel, bool loop, char* sound_name);

	void play(Audio_Source* source);
	void update(Time & time) override;
	void delete_component(Entity entity) override;
};