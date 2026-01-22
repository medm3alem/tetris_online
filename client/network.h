#pragma once
#include <string>

void network_connect(const char* server_ip = "10.90.234.220");
void disconnect();
void network_send(const std::string& msg);
void network_start_listener();
bool network_has_message();
std::string network_pop_message();
bool is_connected();