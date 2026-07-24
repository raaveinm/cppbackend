# TODO

Implement the game join operations and retrieving player information on the game server. A user can join the game on any of the available maps. To join the game, they must provide the server with their dog's nickname and the map ID.

The server must create a dog on the specified map and a player controlling the dog, and then return the player ID and an authentication token to the client. The ID is needed by the client to distinguish its player from others. The client must present the token to the server to receive information about the game session state and to control its player.

## Terminology

* **Dog** — a game object capable of moving around the game map and interacting with other objects according to the game rules.


* **Game Server** (or simply **Server**) — a program running on the server computer. It implements game logic and provides a REST API.


* **Client** — software running in a browser. It can interact with the server and visualize the game state.


* **User** — a person participating in the game using the client software.


* **Player** — an agent inside the game server through which the user can control their dog.


* **Token** — a pseudorandom sequence of characters known only to the user and the server. Therefore, the user can only control their own player, as they do not know the tokens of other players.



## JOIN GAME

To join the game, implement handling of a POST request to the endpoint `/api/v1/game/join`. Request parameters:

* Required header `Content-Type` must be of type `application/json`.


* Request body — a JSON object with required fields `userName` and `mapId`: the player's name and the map ID. The player's name is the same as the dog's name.



Request example:

```text
POST /api/v1/game/join HTTP/1.1
Content-Type: application/json

{"userName": "Scooby Doo", "mapId": "map1"}

```

In case of success, a response with the following properties should be returned:

* Status code `200 OK`.


* Header `Content-Type` must be of type `application/json`.


* Header `Content-Length` must store the size of the response body.


* Required header `Cache-Control` must have the value `no-cache`.


* Response body — a JSON object with fields `authToken` and `playerId`. Field `playerId` — an integer specifying the player ID. Field `authToken` — a token for in-game authorization — a string consisting of 32 random hexadecimal digits.



Response example:

```text
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 61
Cache-Control: no-cache

{"authToken":"6516861d89ebfff147bf2eb2b5153ae1","playerId":0}

```

If a non-existent map ID is specified for `mapId`, a response must be returned with status code `404 Not found` and headers `Content-Length: <response body size>`, `Content-Type: application/json`, and `Cache-Control: no-cache`. Response body — a JSON object with fields `code` and `message`. The `code` field — string `"mapNotFound"`. The `message` field — a human-readable error description string. Example:

```text
HTTP/1.1 404 Not found
Content-Type: application/json
Content-Length: 51
Cache-Control: no-cache

{"code": "mapNotFound", "message": "Map not found"}

```

If an empty player name was provided, a response must be returned with status code `400 Bad request` and headers `Content-Length: <response body size>`, `Content-Type: application/json`, and `Cache-Control: no-cache`. Response body — a JSON object with fields `code` and `message`. The `code` field — string `"invalidArgument"`. The `message` field — a human-readable error description string. Example:

```text
HTTP/1.1 400 Bad request
Content-Type: application/json
Content-Length: 54
Cache-Control: no-cache

{"code": "invalidArgument", "message": "Invalid name"}

```

If an error occurred while parsing JSON or retrieving its properties, a response must be returned with status code `400 Bad request` and headers `Content-Length: <response body size>`, `Content-Type: application/json`, and `Cache-Control: no-cache`. Response body — a JSON object with fields `code` and `message`. The `code` field — string `"invalidArgument"`. The `message` field — a human-readable error description string. Example:

```text
HTTP/1.1 400 Bad request
Content-Type: application/json
Content-Length: 71
Cache-Control: no-cache

{"code": "invalidArgument", "message": "Join game request parse error"}

```

If the request method is not `POST`, a response must be returned with status code `405 Method Not Allowed` and headers `Content-Length: <response body size>`, `Content-Type: application/json`, `Allow: POST`, and `Cache-Control: no-cache`. Response body — a JSON object with fields `code` and `message`. The `code` field — string `"invalidMethod"`, and `message` — a human-readable error description string. Example:

```text
HTTP/1.1 405 Method Not Allowed
Content-Type: application/json
Allow: POST
Content-Length: 68
Cache-Control: no-cache

{"code": "invalidMethod", "message": "Only POST method is expected"} 

```