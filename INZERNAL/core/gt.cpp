#pragma once
#include <core/gt.h>
#include <hooks/SendPacket.h>
#include <hooks/SendPacketRaw.h>
#include <hooks/ProcessTankUpdatePacket.h>
#include <sdk/GameUpdatePacket.h>
#include <sdk/sdk.h>
#include <algorithm>
#include <intrin.h>

std::string gt::generate_mac(const std::string& prefix) {
    std::string x = prefix + ":";
    for (int i = 0; i < 5; i++) {
        x += utils::hex_str(utils::random(0, 255));
        if (i != 4)
            x += ":";
    }
    return x;
}

std::string gt::generate_rid() {
    std::string rid_str;

    for (int i = 0; i < 16; i++)
        rid_str += utils::hex_str(utils::random(0, 255));

    std::transform(rid_str.begin(), rid_str.end(), rid_str.begin(), std::toupper);

    return rid_str;
}

std::string gt::generate_meta() {
    return utils::rnd(utils::random(5, 10)) + ".com";
}

std::string gt::get_random_flag() {
    static bool done = false;
    static std::vector<string> candidates{};

    if (!done) {
        CHAR NPath[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, NPath);
        std::string pattern(string(NPath) + "\\interface\\flags\\*.rttex");
        _WIN32_FIND_DATAA data{};
        HANDLE hFind;
        if ((hFind = FindFirstFileA(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
            do {
                auto temp = string(data.cFileName);
                if (utils::replace(temp, ".rttex", ""))
                    if (temp.length() == 2)
                        candidates.push_back(temp);

            } while (FindNextFileA(hFind, &data) != 0);
            FindClose(hFind);
        }
        done = true;
    }

    return candidates[utils::random(0, candidates.size())];
}

std::string gt::get_type_string(uint8_t type) {
    static const char* types[]{ "PACKET_STATE", "PACKET_CALL_FUNCTION", "PACKET_UPDATE_STATUS", "PACKET_TILE_CHANGE_REQUEST", "PACKET_SEND_MAP_DATA",
        "PACKET_SEND_TILE_UPDATE_DATA", "PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE", "PACKET_TILE_ACTIVATE_REQUEST", "PACKET_TILE_APPLY_DAMAGE",
        "PACKET_SEND_INVENTORY_STATE", "PACKET_ITEM_ACTIVATE_REQUEST", "PACKET_ITEM_ACTIVATE_OBJECT_REQUEST", "PACKET_SEND_TILE_TREE_STATE",
        "PACKET_MODIFY_ITEM_INVENTORY", "PACKET_ITEM_CHANGE_OBJECT", "PACKET_SEND_LOCK", "PACKET_SEND_ITEM_DATABASE_DATA", "PACKET_SEND_PARTICLE_EFFECT",
        "PACKET_SET_ICON_STATE", "PACKET_ITEM_EFFECT", "PACKET_SET_CHARACTER_STATE", "PACKET_PING_REPLY", "PACKET_PING_REQUEST", "PACKET_GOT_PUNCHED",
        "PACKET_APP_CHECK_RESPONSE", "PACKET_APP_INTEGRITY_FAIL", "PACKET_DISCONNECT", "PACKET_BATTLE_JOIN", "PACKET_BATTLE_EVENT", "PACKET_USE_DOOR",
        "PACKET_SEND_PARENTAL", "PACKET_GONE_FISHIN", "PACKET_STEAM", "PACKET_PET_BATTLE", "PACKET_NPC", "PACKET_SPECIAL", "PACKET_SEND_PARTICLE_EFFECT_V2",
        "PACKET_ACTIVE_ARROW_TO_ITEM", "PACKET_SELECT_TILE_INDEX", "PACKET_SEND_PLAYER_TRIBUTE_DATA", "PACKET_SET_EXTRA_MODS", "PACKET_ON_STEP_ON_TILE_MOD",
        "PACKET_ERRORTYPE" };

    if (type >= PACKET_MAXVAL)
        type = PACKET_MAXVAL - 1; //will set any unknown type as errortype and keep us from crashing
    return types[type];
}
#define WORDn(x, n) (*((WORD*)&(x) + n))
int16_t gt::get_cpuid() {
    int32_t regs[4];
    __cpuid((int*)regs, 0);
   
    return WORDn(regs[0], 1) + WORDn(regs[1], 1) + WORDn(regs[2], 1) + WORDn(regs[3], 1) + regs[0] + regs[1] + regs[2] + regs[3];
}

int gt::decrypt_piece(uint8_t* data, uint32_t size, int seed) {

    auto seed_mod = seed;
    char unk = -2 - seed;

    int smth = 0;
    int smth2 = 0;
    do {
        int temp1 = smth;
        int temp2 = *data;
        smth2 = temp1 + temp2;
        *data = unk + temp2;
        --unk;
        ++data;
        smth = seed_mod + smth2;
        ++seed_mod;
        --size;
    } while (size);

    return seed_mod + smth2 - 1;
}


//TLDR gt stores badly encrypted cached versions of hash, wk, mac into registry and they take priority over real one
//this is why unban used to require u to delete the keys too. 
//stealers getting the mac from here would be ideal to use for save decryption, since real mac used by gt cant be known
//and since gt uses mac to encrypt pass (see my save decrypter for decrypter without bruteforce)
void gt::decrypt_reg_vals() 
{
    //did this while bored and it was too easy 
    uint32_t uint3 = (uint16_t)gt::get_cpuid() + 1;
    LPCSTR key = ("Software\\Microsoft\\" + std::to_string(uint3)).c_str();
    BYTE data[1024];
    DWORD data_len = 1024;
    memset(data, 0, 1024);
    if (!utils::read_reg_value(key, (std::to_string((uint3 >> 1))).c_str(), data, &data_len))
        printf("failed at reading reg value!\n");

    int hashc = gt::decrypt_piece(data, data_len - 1, 25532);
    std::string hash = utils::format("%s", data);
    int hash_int = atoi((char*)data);
    memset(data, 0, 1024);
    if (!utils::read_reg_value(key, (std::to_string((uint3 >> 1)) + "c").c_str(), data, &data_len))
        printf("failed at reading reg value!\n");

    auto hashc2 = atol((const char*)data);
    if (!utils::read_reg_value(key, (std::to_string((uint3 >> 1)) + "w").c_str(), data, &data_len))
        printf("failed at reading reg value!\n");

    int wkc = gt::decrypt_piece(data, data_len - 1, 25532);
    std::string wk = utils::format("%s", data);
    memset(data, 0, 1024);
    if (!utils::read_reg_value(key, (std::to_string((uint3 >> 1)) + "wc").c_str(), data, &data_len))
        printf("failed at reading reg value!\n");

    auto wkc2 = atol((const char*)data);
    memset(data, 0, 1024);
    std::string keymac = std::to_string(abs(hash_int / 3));
    std::string valmac = std::to_string(abs(hash_int / 4));

    if (!utils::read_reg_value(keymac.c_str(), valmac.c_str(), data, &data_len))
        printf("failed at reading reg value!\n");
    int macc = gt::decrypt_piece(data, data_len - 1, 25532);
    std::string mac = utils::format("%s", data);

    printf("hash: %s\nwk: %s\nmac: %s\n", hash.c_str(), wk.c_str(), mac.c_str());
    //got bored at this point and couldnt be bothered to read the c for mac too 
}

void gt::send(int type, std::string message, bool hook_send) {
    static auto func = types::SendPacket(sigs::get(sig::sendpacket));
    if (hook_send) //if we want to apply our own code or just log shit
        SendPacketHook::Execute( type, message, sdk::GetPeer());
    else
        func(type, message, sdk::GetPeer());
}

void gt::send(GameUpdatePacket* packet, int extra_size, bool hook_send) {
    static auto func = types::SendPacketRaw(sigs::get(sig::sendpacketraw));
    void* PacketSender = nullptr;
    if (hook_send) //if we want to apply our own code or just log shit
        SendPacketRawHook::Execute(NET_MESSAGE_GAME_PACKET, packet, 56 + extra_size, PacketSender, sdk::GetPeer(), ENET_PACKET_FLAG_RELIABLE);
    else
        func(NET_MESSAGE_GAME_PACKET, packet, 56 + extra_size, PacketSender, sdk::GetPeer(), ENET_PACKET_FLAG_RELIABLE);
}

void gt::send_self(GameUpdatePacket* packet, bool hook_send) {
    static auto func = types::ProcessTankUpdatePacket(sigs::get(sig::processtankupdatepacket));
    if (hook_send) //if we want to apply our own code or just log shit
        ProcessTankUpdatePacketHook::Execute(sdk::GetGameLogic(), packet);
    else
        func(sdk::GetGameLogic(), packet);
}

void gt::send_varlist_self(variantlist_t variantlist, int netid, int delay, bool hook_send) {
    uint32_t data_size = 0;
    void* data = variantlist.serialize_to_mem(&data_size, nullptr);

    auto packet = (GameUpdatePacket*)calloc(sizeof(GameUpdatePacket) + data_size, 1);
    packet->type = PACKET_CALL_FUNCTION;
    packet->netid = netid;
    packet->flags |= 8;
    packet->int_data = delay;
    packet->data_size = data_size;
    memcpy(&packet->data, data, data_size);
    
   static auto func = types::ProcessTankUpdatePacket(sigs::get(sig::processtankupdatepacket));
    if (hook_send) //if we want to apply our own code or just log shit
        ProcessTankUpdatePacketHook::Execute(sdk::GetGameLogic(), packet);
    else
        func(sdk::GetGameLogic(), packet);
    free(packet);
    free(data);

}

bool gt::patch_banbypass() {
    try {
        static auto banbypass = sigs::get(sig::banbypass);
        if (!banbypass) //did not find ban bypass
            throw std::runtime_error("could not find ban bypass");

        auto bypassed = utils::patch_bytes<2>(banbypass, "\x90\x90");
        if (!bypassed)
            throw std::runtime_error("could not patch ban bypass");
        else {
            printf("patched ban bypass\n");
            return true;
        }
    } catch (std::exception exception) {
        printf("exception thrown: %s\n", exception.what());
        utils::read_key();
        return false;
    }
    return false;
}

void gt::hit_tile(CL_Vec2i where) {
    auto local = sdk::GetGameLogic()->GetLocalPlayer();
    if (!local)
        return;

    GameUpdatePacket packet{ 0 };
    packet.type = PACKET_TILE_CHANGE_REQUEST;
    packet.item = 18;
    packet.int_x = where.x;
    packet.int_y = where.y;
    auto pos = local->GetPos();
    packet.pos_x = pos.x;
    packet.pos_y = pos.y;
    gt::send(&packet);
}

void gt::place_tile(int id, CL_Vec2i where) {
    auto local = sdk::GetGameLogic()->GetLocalPlayer();
    if (!local)
        return;

    GameUpdatePacket packet{ 0 };
    packet.type = PACKET_TILE_CHANGE_REQUEST;
    packet.item = id;
    packet.int_x = where.x;
    packet.int_y = where.y;
    auto pos = local->GetPos();
    packet.pos_x = pos.x;
    packet.pos_y = pos.y;
    gt::send(&packet);
}

void gt::wrench_tile(CL_Vec2i where) {
    auto local = sdk::GetGameLogic()->GetLocalPlayer();
    if (!local)
        return;

    GameUpdatePacket packet{ 0 };
    packet.type = PACKET_TILE_CHANGE_REQUEST;
    packet.item = 32;
    packet.int_x = where.x;
    packet.int_y = where.y;
    auto pos = local->GetPos();
    packet.pos_x = pos.x;
    packet.pos_y = pos.y;
    gt::send(&packet);
}
void gt::water_tile(CL_Vec2i where) {
     auto local = sdk::GetGameLogic()->GetLocalPlayer();
    if (!local)
        return;

    GameUpdatePacket packet{ 0 };
    packet.type = PACKET_STATE;
    packet.item = 822;
    packet.int_x = where.x;
    packet.int_y = where.y;
    packet.flags = (1 << 5) | (1 << 10) | (1 << 11); //no enum rn so using raw values
   
    auto pos = local->GetPos();
    packet.pos_x = pos.x;
    packet.pos_y = pos.y;
    gt::send(&packet);
}