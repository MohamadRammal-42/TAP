
This project has been created as part of the 42 curriculum by \<hdaoun\>, \<mrammal\>.

## Description

**TAP**: The Answer Protocol is a shared-world, retro text adventure built around a line-based TCP protocol. A `C++` server hosts a persistent world of interconnected rooms, items, NPCs, and quests, while multiple players connect simultaneously, one through a command-line client, the other through a graphical client, and interact with the world and each other in real time.

The goal of the project is to design and implement a small MUD-style system that exercises systems programming, network programming, concurrency, and clean protocol design. Every player action (moving, taking items, fighting NPCs, chatting, grouping, completing quests) is transmitted as a single UTF-8 line terminated by `\r\n`, and every server reply uses the same framing.

## Instructions

- To run the server:
```bash
make server
```
or manualy
```bash
c++ -Wall -Werror -Wextra -std=c++17 irc.cpp Client.cpp helpers.cpp src/*.cpp world_parser.cpp npcs.cpp -o server.out json_parser.cpp
./server.out
```

- To run the CLI client
```bash
make client
```
or manualy
```bash
c++ -Wall -Werror -Wextra -std=c++17 client_main.cpp -o client.out
./client.out
```

- To run the GUI client
```bash
make client-gui
```
or manualy
```bash
cd gui && rm -rf build && mkdir build && cd build && cmake .. && cmake --build . 
./RPGClient
```

- To remove .out files and the cmake build directory
```bash
make clean
```

## Requirements

- A CLI
- A GUI
- c++(17) or later

### Resources

- `RFC 42TAP`, the attached protocol specification for this project

### AI Usage

- AI was used to generate the initial cod structure for the Qt GUI client
- AI was used to generate the json parser instead of including a build in parser
- Ai was used to structure the README.md

## Architecture

The server is a single-threaded event loop built on `poll(2)`. Every client connection is represented by a `pollfd` entry in a `std::vector<pollfd>`; the listening socket occupies index 0 and each connected client occupies a subsequent slot. `poll()` blocks until at least one descriptor is readable, and the loop then dispatches:

- Listening fd readable → `accept()`, log the peer with `inet_ntop/ntohs`, push a new `pollfd`, send the greeting.

- Client fd readable → `recv()` into a stack buffer, append to that client's per-socket `std::string` buffer, and extract complete `\r\n`-terminated lines.

Command handling is inline (if/else-if chain). Each branch validates the line length, sends a protocol-compliant error if malformed, and performs the action.

Client state (`nickname`, `current_room`, `inventory`, `HP`, `group id`, `quests`) is held in a Client object in a `std::vector<Client>` parallel to the poll set. On disconnect (recv() == 0 or a QUIT), the client is erased from both the poll set and the client list, and a PLAYER_DISCONNECTED log line is emitted. Stale `fds` are removed from `fds` with `erase()` and the loop index is decremented so no event is skipped.

## Protocol Implementation

We implement RFC `42TAP` exactly as specified: `TCP`, `UTF-8`, one message per line, `\r\n` terminated. All commands (`CONNECT`, `LOOK`, `MOVE`, `CHAT`, `TAKE`, `DROP`, `INVENTORY`, `TALK`, `ATTACK`, `STATUS`, `QUEST`, `QUESTS`, `WHO`, `GROUP`, `QUIT`) and all event forms (`EVT GLOBAL CHAT`, `EVT ROOM CHAT`, `EVT GROUP CHAT`, `EVT ROOM PRESENCE ENTER/LEAVE`, `EVT STATS`, `EVT GROUP INVITE/JOIN/LEAVE`) are emitted with the documented prefix.

## Combat System

Combat is turn-resolved on the server, driven by the attacking client's ATTACK <npc> command. There is no separate initiative roll; the attacking player always acts first with a weapon if used, and the target NPC immediately counterattacks if it survives.

## Quest System

Two quest archetypes are supported:

- `eliminate N` of a named `NPC`. Progress is incremented inside the ATTACK handler when the NPC dies: for every active quest with matching type and target, progress += 1.

- `fetch N` of a target `item` (or `coins`). Progress is evaluated at turn-in: the handler counts matching entries in client->inventory (or reads client->getCoins() for currency quests).

## World Design

The map is defined in `world.json` and parsed first to check if none sense exist, then loaded at server startup with a JSON parser, no external dependencies. World contents are deliberately not shown here (they are data, not code); the relevant structural facts are:

- `locations`: interconnected locations forming one or more loops with at least one optional dead-end branch. Exits are stored as a directional map (north, south, east, west, …) whose values must resolve to existing room names; a dedicated validator (parseworld) rejects the map at boot if any exit or item reference is invalid.

- `npcs`: Roles are trader/quest-giver (traders), a boss (combat target with drops), and a passive dialogue-only NPC. The server spawns NPCs per room via spawns: [{ npc_type, count }]. Each NPC carries its own HP, damage, drop table, and for traders a single quest definition and reward table.

- `items`: Distinct items split across consumables, weapons, and armors. Items exist as unique instances: a room's items array is the authoritative source; taking one removes it from the array, dropping one pushes it back, and a periodic clear_lag (every 120 s) restores any drift from the original world snapshot.

## Server Logging

Logging is structured JSON, one object per line, printed to stdout. Every log record is produced by `printJson(log)` after manually populating a `JsonObject` and includes:

- `timestamp`, (`YYYY-MM-DDTHH:MM:SSZ`, via `gmtime` + `strftime`)

- `level`, "`INFO`", "`WARN`", or "`ERROR`"

- `event`, the event name or the raw command line (`PLAYER_CONNECTED`, `LOOK`, `INVALID_COMMAND`, `QUEST_PROGRESS`, `LEVEL_UP`)

- `response`, the exact string returned to the client (or the reason for a WARN/ERROR)

- plus context-dependent keys: `player`, `ip`, `port`, `from`, `to`, `group`, `progress`, `target`, etc.

Event categories covered:

- Connection lifecycle: `PLAYER_CONNECTED`, `PLAYER_DISCONNECTED`, `NAME_IN_USE`, `ALREADY_CONNECTED`

- Command receipt: every dispatched command branch logs its `event = line` and the `response`

Errors: `INVALID_COMMAND` (911), `UNAUTHORIZED_ACTION` (710), `NO_EXIT` (301), `ITEM_NOT_FOUND` (404), `NPC_NOT_FOUND` (404), `NOT_IN_GROUP` (401), `ALREADY_IN_GROUP` (402), `NO_QUEST_AVAILABLE` (406)

World state changes: `MOVE` (with `from`/`to`), `TAKE`, `DROP`, `USE`, `UNUSE`, `ATTACK`, `LEVEL_UP`, `QUEST_PROGRESS`

Chat: `CHAT_GLOBAL`, `CHAT_ROOM`, `CHAT_GROUP` with the player and message

Group actions: `GROUP_CREATE`, `GROUP_INVITE`, `GROUP_JOIN`, `GROUP_LEAVE`

## Group Contributions

All the logical part was done together, all the commands and the irc part was on the same workstation, The only differences:

- `mrammal`: README.md, Makefile, ParseWorld
- `hdaoun` : The GUI-client

## Building and Running

- To run the server:
```bash
make server
```
or manualy
```bash
c++ -Wall -Werror -Wextra -std=c++17 irc.cpp Client.cpp helpers.cpp src/*.cpp world_parser.cpp npcs.cpp -o server.out json_parser.cpp
./server.out
```

- To run the CLI client
```bash
make client
```
or manualy
```bash
c++ -Wall -Werror -Wextra -std=c++17 client_main.cpp -o client.out
./client.out
```

- To run the GUI client
```bash
make client-gui
```
or manualy
```bash
cd gui && rm -rf build && mkdir build && cd build && cmake .. && cmake --build . 
./RPGClient
```

- To clean
```bash
make clean
```

## Testing

- Start the server in one terminal.
- Start the CLI client and type `CONNECT alice`.
- Start the GUI client (or a second CLI) and type `CONNECT bob`.
- In both, run `WHO` — both should see each other in room and the correct server total.
- Have `alice` run `MOVE north` — `bob` should see `EVT ROOM PRESENCE LEAVE alice` and then, if he follows, `EVT ROOM PRESENCE ENTER alice`.
- Have `alice` run `CHAT GLOBAL hello` — `bob` sees `EVT GLOBAL CHAT alice: hello`, and vice versa.
- Have `alice` run `CHAT ROOM hello` — only players in the same room see it.
- Have `alice` run `GROUP CREATE` — she receives `OK group=<id>`. Then `GROUP INVITE bob`, and `bob` runs `GROUP JOIN <id>`. Both can now use `CHAT GROUP`.
- Kill the server with Ctrl-C while both clients are connected — both clients should print a disconnect notice and stop cleanly (no hang). Restart the server; reconnecting should work because `SO_REUSEADDR` is set.
- Move a character into a room containing a boss-type NPC.
- `STATUS` — HP, attack power, and defense are shown.
- `ATTACK <npc>` — the reply carries the current combat state (attacker HP, target HP, damage). Repeating the command should reduce the NPC's HP until it dies; the player then sees the elimination message and drops.
- Take enough damage from a counterattacking boss to reach 0 HP — the player should be teleported to the world's first room with `max_hp / 2`.
- `INVENTORY` after a kill — dropped items should be listed.
- `USE <consumable>` at full HP does not over-heal; below max HP it heals up to `max_hp`.
- `USE <weapon>` swaps in the new weapon and moves the previous one back into inventory; `UNUSE <weapon>` reverses this. Same for armor.
- `TALK <npc>` — verify dialogue is returned.
- `QUEST <npc>` — the response contains `quest_id`, `description`, `type`, `target`, `required`, `provider`, `progress`.
- `QUESTS` — the quest appears with current progress.
- For an `eliminate` quest: kill the required NPC `N` times. `QUESTS` should show `progress = N`. Walk back to the provider and run `TALK <npc>` — the reply is the `qc_dialogue` and the reward JSON; the quest disappears from `QUESTS`.
- For a `fetch` quest: gather the required items or coins and run `TALK <npc>` at the provider — same outcome.
- Requesting a quest twice from the same NPC returns `ERR 406 NO_QUEST_AVAILABLE` the second time.