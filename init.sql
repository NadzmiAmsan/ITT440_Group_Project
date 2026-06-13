-- =============================================================
-- ITT440 Network Programming - Group Project
-- Database Initialization Script
-- Author  : Nadzmi
-- Purpose : Creates and seeds the itt440_db database
-- =============================================================

-- ── 1. DATABASE ───────────────────────────────────────────────
CREATE DATABASE IF NOT EXISTS itt440_db
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE itt440_db;

-- ── 2. MAIN TABLE ─────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS socket_data (
    id             INT          AUTO_INCREMENT PRIMARY KEY,
    user           VARCHAR(100) NOT NULL UNIQUE          COMMENT 'Unique server identifier',
    points         INT          NOT NULL DEFAULT 0       COMMENT 'Accumulated points, incremented every 30s',
    datetime_stamp DATETIME     NOT NULL
                   DEFAULT CURRENT_TIMESTAMP
                   ON UPDATE CURRENT_TIMESTAMP           COMMENT 'Timestamp of last update'
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COMMENT='Live socket server leaderboard';

-- ── 3. AUDIT / HISTORY TABLE ──────────────────────────────────
CREATE TABLE IF NOT EXISTS socket_data_history (
    history_id     INT          AUTO_INCREMENT PRIMARY KEY,
    user           VARCHAR(100) NOT NULL                 COMMENT 'Server that triggered the update',
    points_before  INT          NOT NULL                 COMMENT 'Points value before this update',
    points_after   INT          NOT NULL                 COMMENT 'Points value after this update',
    updated_at     DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'When the update happened'
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COMMENT='Full audit trail of every points update';

-- ── 4. TRIGGER: auto-log every UPDATE ─────────────────────────
CREATE TRIGGER trg_after_points_update
AFTER UPDATE ON socket_data
FOR EACH ROW
INSERT INTO socket_data_history (user, points_before, points_after, updated_at)
VALUES (NEW.user, OLD.points, NEW.points, NOW());

-- ── 5. VIEW: leaderboard ──────────────────────────────────────
CREATE OR REPLACE VIEW leaderboard AS
    SELECT
        RANK() OVER (ORDER BY points DESC) AS `rank`,
        user,
        points,
        datetime_stamp AS last_update
    FROM socket_data
    ORDER BY points DESC;

-- ── 6. SEED DATA ──────────────────────────────────────────────
INSERT INTO socket_data (user, points, datetime_stamp) VALUES
    ('c_server_user',        0, NOW()),
    ('python_server_user_1', 0, NOW()),
    ('python_server_user_2', 0, NOW())
ON DUPLICATE KEY UPDATE user = user;

-- ── 7. VERIFY ─────────────────────────────────────────────────
SELECT '=== Tables Created ===' AS info;
SHOW TABLES;

SELECT '=== socket_data ===' AS info;
SELECT * FROM socket_data;

SELECT '=== leaderboard ===' AS info;
SELECT * FROM leaderboard;
