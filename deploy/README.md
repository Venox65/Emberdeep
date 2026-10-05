# Emberdeep production deployment

1. Copy `.env.example` to `.env`.
2. Replace every placeholder with a strong secret. Do not commit `.env`.
3. Set `EMBERDEEP_DOMAIN` to a DNS name pointing at the host.
4. Start the stack:

```bash
docker compose up -d --build
```

Caddy obtains and renews HTTPS certificates automatically when the domain resolves
to the host and ports 80/443 are reachable.

The PostgreSQL bootstrap account (`POSTGRES_ADMIN_*`) is used only by the database
image during first initialization. The API connects with the separate
`POSTGRES_APP_*` role, which is not a superuser. If the database volume already
exists, changing initialization variables does not recreate roles; migrate or
recreate the volume deliberately during a controlled deployment.

For an existing volume created before the split-role configuration, run the
upgrade script while the database is running. Replace `emberdeep` below with
the old PostgreSQL role if it differs:

```bash
./server/postgres-init/upgrade-existing-volume.sh \
  game-db-1 emberdeep emberdeep .env
docker compose restart api
```

Operational checks:

```bash
curl https://api.example.com/healthz
curl https://api.example.com/readyz
docker compose logs -f api
```

Back up PostgreSQL regularly:

```bash
DB=${POSTGRES_DB:-emberdeep}
docker compose exec -T db pg_dump \
  -U "$POSTGRES_ADMIN_USER" "$DB" > emberdeep-$(date +%F).sql
```

The API container is intentionally stateless. PostgreSQL and the named volumes
are the durable state. Put the database volume and backups on encrypted storage.
