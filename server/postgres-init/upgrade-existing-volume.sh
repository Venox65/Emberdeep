#!/bin/bash
set -Eeuo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 <container> <legacy-role> <database> <env-file>" >&2
  exit 2
fi

container="$1"
legacy_role="$2"
database="$3"
env_file="$4"

set -a
# shellcheck disable=SC1090
. "$env_file"
set +a

: "${POSTGRES_ADMIN_USER:?POSTGRES_ADMIN_USER is required}"
: "${POSTGRES_ADMIN_PASSWORD:?POSTGRES_ADMIN_PASSWORD is required}"
: "${POSTGRES_APP_USER:?POSTGRES_APP_USER is required}"
: "${POSTGRES_APP_PASSWORD:?POSTGRES_APP_PASSWORD is required}"

docker exec -i "$container" psql -v ON_ERROR_STOP=1 \
  -U "$legacy_role" -d "$database" \
  --set admin_user="$POSTGRES_ADMIN_USER" \
  --set admin_password="$POSTGRES_ADMIN_PASSWORD" \
  --set app_user="$POSTGRES_APP_USER" \
  --set app_password="$POSTGRES_APP_PASSWORD" \
  --set app_db="$database" <<'SQL'
CREATE EXTENSION IF NOT EXISTS citext;
CREATE EXTENSION IF NOT EXISTS pgcrypto;

SELECT format('CREATE ROLE %I LOGIN SUPERUSER PASSWORD %L', :'admin_user', :'admin_password')
WHERE NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = :'admin_user') \gexec
SELECT format('ALTER ROLE %I WITH LOGIN SUPERUSER PASSWORD %L', :'admin_user', :'admin_password')
WHERE EXISTS (SELECT 1 FROM pg_roles WHERE rolname = :'admin_user') \gexec
SELECT format('CREATE ROLE %I LOGIN NOSUPERUSER PASSWORD %L', :'app_user', :'app_password')
WHERE NOT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = :'app_user') \gexec
SELECT format('ALTER ROLE %I WITH LOGIN NOSUPERUSER PASSWORD %L', :'app_user', :'app_password')
WHERE EXISTS (SELECT 1 FROM pg_roles WHERE rolname = :'app_user') \gexec

REVOKE CREATE ON DATABASE :"app_db" FROM PUBLIC;
GRANT CONNECT ON DATABASE :"app_db" TO :"app_user";
GRANT USAGE, CREATE ON SCHEMA public TO :"app_user";
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO :"app_user";
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO :"app_user";
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL ON TABLES TO :"app_user";
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL ON SEQUENCES TO :"app_user";
ALTER TABLE IF EXISTS access_tokens ADD COLUMN IF NOT EXISTS revoked_at TIMESTAMPTZ;
GRANT SELECT, INSERT, UPDATE, DELETE ON access_tokens TO :"app_user";
ALTER TABLE IF EXISTS accounts OWNER TO :"app_user";
ALTER TABLE IF EXISTS refresh_tokens OWNER TO :"app_user";
ALTER TABLE IF EXISTS access_tokens OWNER TO :"app_user";
ALTER TABLE IF EXISTS saves OWNER TO :"app_user";
ALTER TABLE IF EXISTS schema_migrations OWNER TO :"app_user";
SQL
