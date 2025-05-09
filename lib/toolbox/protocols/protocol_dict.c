#include <furi.h>
#include "protocol_dict.h"

struct ProtocolDict {
    const ProtocolBase* const* base;
    size_t count;
    void* data[];
};

ProtocolDict* protocol_dict_alloc(const ProtocolBase* const* protocols, size_t count) {
    furi_check(protocols);

    ProtocolDict* dict = malloc(sizeof(ProtocolDict) + (sizeof(void*) * count));
    dict->base = protocols;
    dict->count = count;

    for(size_t i = 0; i < dict->count; i++) {
        dict->data[i] = dict->base[i]->alloc();
    }

    return dict;
}

void protocol_dict_free(ProtocolDict* dict) {
    furi_check(dict);

    for(size_t i = 0; i < dict->count; i++) {
        dict->base[i]->free(dict->data[i]);
    }

    free(dict);
}

void protocol_dict_set_data(
    ProtocolDict* dict,
    size_t protocol_index,
    const uint8_t* data,
    size_t data_size) {
    furi_check(protocol_index < dict->count);
    furi_check(dict->base[protocol_index]->get_data != NULL);

    uint8_t* protocol_data = dict->base[protocol_index]->get_data(dict->data[protocol_index]);
    size_t protocol_data_size = dict->base[protocol_index]->data_size;
    furi_check(data_size >= protocol_data_size);
    memcpy(protocol_data, data, protocol_data_size);
}

void protocol_dict_get_data(
    ProtocolDict* dict,
    size_t protocol_index,
    uint8_t* data,
    size_t data_size) {
    furi_check(protocol_index < dict->count);
    furi_check(dict->base[protocol_index]->get_data != NULL);

    uint8_t* protocol_data = dict->base[protocol_index]->get_data(dict->data[protocol_index]);
    size_t protocol_data_size = dict->base[protocol_index]->data_size;
    furi_check(data_size >= protocol_data_size);
    memcpy(data, protocol_data, protocol_data_size);
}

size_t protocol_dict_get_data_size(ProtocolDict* dict, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    return dict->base[protocol_index]->data_size;
}

size_t protocol_dict_get_max_data_size(ProtocolDict* dict) {
    furi_check(dict);
    size_t max_data_size = 0;
    for(size_t i = 0; i < dict->count; i++) {
        size_t data_size = dict->base[i]->data_size;
        if(data_size > max_data_size) {
            max_data_size = data_size;
        }
    }

    return max_data_size;
}

const char* protocol_dict_get_name(ProtocolDict* dict, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    return dict->base[protocol_index]->name;
}

const char* protocol_dict_get_manufacturer(ProtocolDict* dict, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    return dict->base[protocol_index]->manufacturer;
}

void protocol_dict_decoders_start(ProtocolDict* dict) {
    furi_check(dict);

    for(size_t i = 0; i < dict->count; i++) {
        ProtocolDecoderStart fn = dict->base[i]->decoder.start;

        if(fn) {
            fn(dict->data[i]);
        }
    }
}

uint32_t protocol_dict_get_features(ProtocolDict* dict, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    return dict->base[protocol_index]->features;
}

ProtocolId protocol_dict_decoders_feed(ProtocolDict* dict, bool level, uint32_t duration) {
    furi_check(dict);

    bool done = false;
    ProtocolId ready_protocol_id = PROTOCOL_NO;

    for(size_t i = 0; i < dict->count; i++) {
        ProtocolDecoderFeed fn = dict->base[i]->decoder.feed;

        if(fn) {
            if(fn(dict->data[i], level, duration)) {
                if(!done) {
                    ready_protocol_id = i;
                    done = true;
                }
            }
        }
    }

    return ready_protocol_id;
}

ProtocolId protocol_dict_decoders_feed_by_feature(
    ProtocolDict* dict,
    uint32_t feature,
    bool level,
    uint32_t duration) {
    furi_check(dict);

    bool done = false;
    ProtocolId ready_protocol_id = PROTOCOL_NO;

    for(size_t i = 0; i < dict->count; i++) {
        uint32_t features = dict->base[i]->features;
        if(features & feature) {
            ProtocolDecoderFeed fn = dict->base[i]->decoder.feed;

            if(fn) {
                if(fn(dict->data[i], level, duration)) {
                    if(!done) {
                        ready_protocol_id = i;
                        done = true;
                    }
                }
            }
        }
    }

    return ready_protocol_id;
}

ProtocolId protocol_dict_decoders_feed_by_id(
    ProtocolDict* dict,
    size_t protocol_index,
    bool level,
    uint32_t duration) {
    furi_check(protocol_index < dict->count);

    ProtocolId ready_protocol_id = PROTOCOL_NO;
    ProtocolDecoderFeed fn = dict->base[protocol_index]->decoder.feed;

    if(fn) {
        if(fn(dict->data[protocol_index], level, duration)) {
            ready_protocol_id = protocol_index;
        }
    }

    return ready_protocol_id;
}

bool protocol_dict_encoder_start(ProtocolDict* dict, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    ProtocolEncoderStart fn = dict->base[protocol_index]->encoder.start;

    if(fn) {
        return fn(dict->data[protocol_index]);
    } else {
        return false;
    }
}

LevelDuration protocol_dict_encoder_yield(ProtocolDict* dict, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    ProtocolEncoderYield fn = dict->base[protocol_index]->encoder.yield;

    if(fn) {
        return fn(dict->data[protocol_index]);
    } else {
        return level_duration_reset();
    }
}

void protocol_dict_render_uid(ProtocolDict* dict, FuriString* result, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    ProtocolRenderData fn = dict->base[protocol_index]->render_uid;

    if(fn) {
        fn(dict->data[protocol_index], result);
    }
}

void protocol_dict_render_data(ProtocolDict* dict, FuriString* result, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    ProtocolRenderData fn = dict->base[protocol_index]->render_data;

    if(fn) {
        fn(dict->data[protocol_index], result);
    }
}

void protocol_dict_render_brief_data(ProtocolDict* dict, FuriString* result, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    ProtocolRenderData fn = dict->base[protocol_index]->render_brief_data;

    if(fn) {
        fn(dict->data[protocol_index], result);
    }
}

uint32_t protocol_dict_get_validate_count(ProtocolDict* dict, size_t protocol_index) {
    furi_check(protocol_index < dict->count);
    return dict->base[protocol_index]->validate_count;
}

ProtocolId protocol_dict_get_protocol_by_name(ProtocolDict* dict, const char* name) {
    furi_check(dict);
    furi_check(name);
    for(size_t i = 0; i < dict->count; i++) {
        if(strcmp(name, protocol_dict_get_name(dict, i)) == 0) {
            return i;
        }
    }
    return PROTOCOL_NO;
}

bool protocol_dict_get_write_data(ProtocolDict* dict, size_t protocol_index, void* data) {
    furi_check(protocol_index < dict->count);
    ProtocolWriteData fn = dict->base[protocol_index]->write_data;

    furi_check(fn);
    return fn(dict->data[protocol_index], data);
}
