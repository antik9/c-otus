#define _POSIX_C_SOURCE 200809L

#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

#define METAWEATHER_QUERY_LOCATION \
    "https://www.metaweather.com/api/location/search/?query="
#define METAWEATHER_QUERY_WEATHER "https://www.metaweather.com/api/location/"

#define WEATHER_MSG_LENGTH 1024

struct Location {
    int id;
    char* title;
} Location;

char* prepare_url(char* base, char* params) {
    char* full_url = malloc(128 * sizeof(char));
    snprintf(full_url, 128, "%s%s", base, params);
    return full_url;
}

CURLcode make_request(char* full_url, struct MemoryChunk* chunk) {
    CURL* curl_handle = curl_easy_init();
    if (curl_handle) {
        curl_easy_setopt(curl_handle, CURLOPT_URL, full_url);
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)chunk);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        CURLcode res = curl_easy_perform(curl_handle);

        curl_easy_cleanup(curl_handle);
        return res;
    }

    return CURLE_FAILED_INIT;
}

struct Location* get_location(char* location_query) {
    struct Location* location = malloc(sizeof(struct Location));

    struct MemoryChunk chunk;
    chunk.size = 0;
    chunk.memory = malloc(1);
    struct cJSON* json = NULL;

    char* full_url = prepare_url(METAWEATHER_QUERY_LOCATION, location_query);
    if (full_url == NULL) {
        fprintf(stderr, "cannot allocate space for location URL\n");
        goto fail;
    }

    CURLcode res = make_request(full_url, &chunk);
    if (res != CURLE_OK) {
        fprintf(stderr, "cannot get location: %s\n", curl_easy_strerror(res));
        goto fail;
    }

    json = cJSON_Parse(chunk.memory);
    if (json == NULL) {
        fprintf(stderr, "cannot parse location json\n");
        goto fail;
    }
    cJSON* location_json = cJSON_GetArrayItem(json, 0);
    if (location_json == NULL) {
        fprintf(stderr, "cannot find given location\n");
        goto fail;
    }
    cJSON* id_json = cJSON_GetObjectItem(location_json, "woeid");
    if (id_json == NULL) {
        fprintf(stderr, "location doesn't have id field\n");
        goto fail;
    }
    cJSON* title_json = cJSON_GetObjectItem(location_json, "title");
    if (id_json == NULL) {
        fprintf(stderr, "location doesn't have title field\n");
        goto fail;
    }

    location->id = id_json->valueint;
    location->title = strdup(title_json->valuestring);

    goto end;

fail:
    free(location);
    location = NULL;

end:
    free(chunk.memory);
    free(full_url);
    if (json != NULL) cJSON_Delete(json);

    return location;
}

char* get_weather_for_location(struct Location* location) {
    char* weather = NULL;

    struct MemoryChunk chunk;
    chunk.size = 0;
    chunk.memory = malloc(1);
    struct cJSON* json = NULL;

    char* location_id_param = calloc(16, sizeof(char));
    snprintf(location_id_param, 16, "%d", location->id);
    location_id_param[strlen(location_id_param)] = '/';

    char* full_url = prepare_url(METAWEATHER_QUERY_WEATHER, location_id_param);
    if (full_url == NULL) {
        fprintf(stderr, "cannot allocate space for weather URL\n");
        goto fail;
    }

    CURLcode res = make_request(full_url, &chunk);
    if (res != CURLE_OK) {
        fprintf(stderr, "cannot get weather: %s\n", curl_easy_strerror(res));
        goto fail;
    }

    json = cJSON_Parse(chunk.memory);
    if (json == NULL) {
        fprintf(stderr, "cannot parse weather json:\n");
        goto fail;
    }
    cJSON* consolicated_json =
        cJSON_GetObjectItem(json, "consolidated_weather");
    if (consolicated_json == NULL) {
        fprintf(stderr,
                "response have no consolidated information about weather\n");
        goto fail;
    }
    cJSON* forecast_json = cJSON_GetArrayItem(consolicated_json, 0);
    if (forecast_json == NULL) {
        fprintf(stderr, "response doesn't contains forecasts for today\n");
        goto fail;
    }

    weather = calloc(WEATHER_MSG_LENGTH, sizeof(char));
    sprintf(weather, "\t⛅ Weather in %s ⛅\n", location->title);

    cJSON* weather_description =
        cJSON_GetObjectItem(forecast_json, "weather_state_name");
    if (weather_description == NULL) {
        fprintf(stderr, "forecast doesn't contain weather description\n");
        goto fail;
    }
    snprintf(weather + strlen(weather), WEATHER_MSG_LENGTH - strlen(weather),
             "Overall description\t%s\n", weather_description->valuestring);

    cJSON* wind_speed = cJSON_GetObjectItem(forecast_json, "wind_speed");
    if (wind_speed == NULL) {
        fprintf(stderr, "forecast doesn't contain wind speed\n");
        goto fail;
    }
    snprintf(weather + strlen(weather), WEATHER_MSG_LENGTH - strlen(weather),
             "Wind speed\t\t%.2f mph\n", wind_speed->valuedouble);

    cJSON* wind_direction =
        cJSON_GetObjectItem(forecast_json, "wind_direction");
    if (wind_direction == NULL) {
        fprintf(stderr, "forecast doesn't contain wind direction\n");
        goto fail;
    }
    snprintf(weather + strlen(weather), WEATHER_MSG_LENGTH - strlen(weather),
             "Wind direction\t\t%.2f degrees\n", wind_direction->valuedouble);

    cJSON* min_temp = cJSON_GetObjectItem(forecast_json, "min_temp");
    if (min_temp == NULL) {
        fprintf(stderr, "forecast doesn't contain minimal temperature\n");
        goto fail;
    }
    snprintf(weather + strlen(weather), WEATHER_MSG_LENGTH - strlen(weather),
             "Min temperature\t\t%.2f ℃\n", min_temp->valuedouble);

    cJSON* max_temp = cJSON_GetObjectItem(forecast_json, "max_temp");
    if (max_temp == NULL) {
        fprintf(stderr, "forecast doesn't contain maximal temperature\n");
        goto fail;
    }
    snprintf(weather + strlen(weather), WEATHER_MSG_LENGTH - strlen(weather),
             "Max temperature\t\t%.2f ℃", max_temp->valuedouble);
    goto end;

fail:
    if (weather != NULL) {
        free(weather);
        weather = NULL;
    }

end:
    free(chunk.memory);
    free(full_url);
    free(location_id_param);

    if (json != NULL) cJSON_Delete(json);

    return weather;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage:\n    weather-app <location>\n");
        return 1;
    }

    int exit_code = 0;
    char* weather = NULL;

    struct Location* location = get_location(argv[1]);
    if (location == NULL) {
        exit_code = 1;
        goto end;
    }

    weather = get_weather_for_location(location);
    if (weather == NULL) {
        exit_code = 1;
        goto end;
    }

    puts(weather);

end:
    if (location != NULL) {
        free(location->title);
        free(location);
    }
    if (weather != NULL) free(weather);

    return exit_code;
}
