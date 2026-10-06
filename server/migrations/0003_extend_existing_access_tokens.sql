DO $$
BEGIN
  IF NOT EXISTS (SELECT 1 FROM schema_migrations WHERE version = 3) THEN
    UPDATE access_tokens
    SET expires_at = now() + interval '30 days'
    WHERE revoked_at IS NULL;

    INSERT INTO schema_migrations(version) VALUES (3);
  END IF;
END
$$;
