# Emberdeep: The Hollow Crown

Emberdeep is an offline-first C++20/raylib procedural clicker game. It uses
procedural shapes instead of sprites and keeps game rules in the shared `core`
library so the terminal playtest, graphical client, and backend use the same
logic.

## Requirements

- C++20 compiler
- CMake 3.25+
- raylib
- OpenSSL
- Docker Desktop or another Docker Engine for the production API stack

On macOS with Homebrew:

```bash
brew install cmake raylib openssl
```

## Build and test locally

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

Run the graphical client:

```bash
./build/client/emberdeep_graphical
```

Run the terminal playtest:

```bash
./build/client/emberdeep_playtest
```

## Backend deployment

The production profile uses PostgreSQL 16, the C++ API, and Caddy for HTTPS.

```bash
cp .env.example .env
# Edit .env and replace every placeholder with a strong secret.
docker compose up -d --build --pull always
```

Check the services:

```bash
curl https://your-domain.example/healthz
curl https://your-domain.example/readyz
docker compose ps
docker compose logs -f api
```

For local client playtesting while the containers are running, the API is
available at `http://127.0.0.1:8080`.

Existing database volumes created before the split PostgreSQL roles need the
one-time upgrade script documented in `deploy/README.md`.

PostgreSQL data lives in the `postgres_data` volume, and API fallback state lives
in `api_data`, so recreating containers does not remove saved data. Authenticated
sessions default to 30 days (`EMBERDEEP_ACCESS_TOKEN_TTL_SECONDS`); migration
`0003` extends existing active sessions once so a server relaunch does not
invalidate the token stored by the desktop client.

## Accounts and local saves

Guest progress is stored locally. Signed-in progress is stored locally and
synchronized to the backend. Local saves are username-bound:

```text
emberdeep-guest.sav
emberdeep-<username>.sav
```

Signing out does not delete any save. The next account can sign in or play as a
guest, and switching back loads that account's local save before cloud sync.
The account profile is stored in `emberdeep.profile` and is ignored by Git.

## Inspecting PostgreSQL data

Open a `psql` shell as the administrator:

```bash
set -a
. .env
set +a
DB=${POSTGRES_DB:-emberdeep}
docker compose exec -T \
  -e PGPASSWORD="$POSTGRES_ADMIN_PASSWORD" \
  db psql -h 127.0.0.1 \
  -U "$POSTGRES_ADMIN_USER" \
  -d "$DB"
```

Useful queries:

```sql
\dt
\du
SELECT id, public_id, username, status, created_at, last_seen_at
FROM accounts
ORDER BY id;

SELECT account_id, revision, schema_version, progress_score, created_at
FROM saves
ORDER BY account_id, revision DESC;

SELECT count(*) AS account_count FROM accounts;
SELECT count(*) AS save_revision_count FROM saves;
\q
```

One-off commands without opening a shell:

```bash
docker compose exec -T \
  -e PGPASSWORD="$POSTGRES_ADMIN_PASSWORD" \
  db psql -h 127.0.0.1 -U "$POSTGRES_ADMIN_USER" -d "$DB" \
  -c 'SELECT username, status, created_at FROM accounts ORDER BY id;'
```

Back up the database:

```bash
docker compose exec -T \
  -e PGPASSWORD="$POSTGRES_ADMIN_PASSWORD" \
  db pg_dump -h 127.0.0.1 -U "$POSTGRES_ADMIN_USER" "$DB" \
  > "emberdeep-$(date +%F).sql"
```

## Repository layout

| Path | Purpose |
|---|---|
| `core/` | Shared game rules, data models, saves, stats, and cloud client |
| `client/` | Graphical and terminal clients |
| `server/` | API, PostgreSQL backend, migrations, and database scripts |
| `tests/` | Catch2 core and persistence tests |
| `data/` | Data-driven materials, layers, bosses, and dungeon definitions |
| `deploy/` | Caddy and deployment documentation |

Never commit `.env`, database dumps, local save files, profile files, or
generated build output.
