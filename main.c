#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <curl/curl.h>

static const char *URL = "https://...";
static const int SLEEP_INTERVAL = 1;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    printf("%.*s\n", (int)totalSize, (char *)contents);
    return totalSize;
}

int main(void)
{
    CURLM *multi_handle;
    CURL *easy_handle;
    int still_running; 
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    multi_handle = curl_multi_init();

    easy_handle = curl_easy_init();
    if (!easy_handle)
    {
        fprintf(stderr, "Failed to create easy handle\n");
        return 1;
    }
    // Set Content-Type header
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    const char *postData = "{}";

    for (;;)
    {
        curl_easy_setopt(easy_handle, CURLOPT_URL, URL);
        curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(easy_handle, CURLOPT_POST, 1L);
        curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, postData);
        curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, headers);
        // Add the easy handle to the multi handle
        curl_multi_add_handle(multi_handle, easy_handle);
        // Perform the request
        curl_multi_perform(multi_handle, &still_running);

        // Wait for the request to complete
        while (still_running)
        {
            int numfds;
            curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
            // printf("...%d\n", numfds);
            curl_multi_perform(multi_handle, &still_running);
        }

        // Get the IP address
        long resp_code = 0;
        res = curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &resp_code);
        assert(res == CURLE_OK);
        char *ip;
        res = curl_easy_getinfo(easy_handle, CURLINFO_PRIMARY_IP, &ip); 
        if (res == CURLE_OK && ip)
        {
            printf("%ld %s\n", resp_code, ip);
        }
        else
        {
            fprintf(stderr, "Failed to get IP address: %s\n", curl_easy_strerror(res));
        }

        // Reset the handle for the next request
        curl_multi_remove_handle(multi_handle, easy_handle);
        curl_easy_reset(easy_handle);
        sleep(SLEEP_INTERVAL);
    }

    // Cleanup
    curl_multi_cleanup(multi_handle);
    curl_easy_cleanup(easy_handle);
    curl_global_cleanup();

    return 0;
}
