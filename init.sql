-- =============================================================
-- ITT440 Network Programming - Group Project
-- THEME: "The Docker Diner" 🍳
-- Database Initialization Script
-- Author  : Nadzmi
-- =============================================================
-- KITCHEN STORY:
-- This database is the Diner's ORDER BOOK.
-- 3 Chefs (C Chef, Py Chef 1, Py Chef 2) each run their own
-- isolated kitchen station (Docker container). Every 30 seconds,
-- each Chef finishes one dish and logs it in the Order Book.
-- Waiters (clients) walk up and ask "how many dishes so far?"
-- =============================================================

-- ── 1. THE DINER (DATABASE) ────────────────────────────────────
CREATE DATABASE IF NOT EXISTS itt440_db
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE itt440_db;

-- ── 2. THE ORDER BOOK (MAIN TABLE) ─────────────────────────────
-- Every Chef has one row here showing their dish count
-- and the time of their last cooked dish.
CREATE TABLE IF NOT EXISTS socket_data (
    id             INT          AUTO_INCREMENT PRIMARY KEY,
    user           VARCHAR(100) NOT NULL UNIQUE          COMMENT 'Chef identifier (which kitchen station)',
    points         INT          NOT NULL DEFAULT 0       COMMENT 'Dishes cooked so far, +1 every 30s',
    datetime_stamp DATETIME     NOT NULL
                   DEFAULT CURRENT_TIMESTAMP
                   ON UPDATE CURRENT_TIMESTAMP           COMMENT 'Time of the last dish cooked'
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COMMENT='The Diner Order Book - tracks every Chef live';

-- ── 3. KITCHEN ACTIVITY LOG (HISTORY TABLE) ────────────────────
-- A full record of every single dish ever cooked by every Chef -
-- like a kitchen CCTV recording every order made.
CREATE TABLE IF NOT EXISTS socket_data_history (
    history_id     INT          AUTO_INCREMENT PRIMARY KEY,
    user           VARCHAR(100) NOT NULL                 COMMENT 'Which Chef cooked this dish',
    points_before  INT          NOT NULL                 COMMENT 'Dish count before cooking',
    points_after   INT          NOT NULL                 COMMENT 'Dish count after cooking',
    updated_at     DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Exact time the dish was logged'
) ENGINE=InnoDB
  DEFAULT CHARSET=utf8mb4
  COMMENT='Kitchen Activity Log - CCTV record of every dish cooked';

-- ── 4. THE KITCHEN CCTV (TRIGGER) ──────────────────────────────
-- Automatically "films" every dish update - the moment a Chef's
-- dish count changes, this trigger writes it into the Activity Log.
CREATE TRIGGER trg_after_points_update
AFTER UPDATE ON socket_data
FOR EACH ROW
INSERT INTO socket_data_history (user, points_before, points_after, updated_at)
VALUES (NEW.user, OLD.points, NEW.points, NOW());

-- ── 5. TOP CHEF BOARD (VIEW) ────────────────────────────────────
-- The Diner's wall-mounted "Top Chef of the Day" board,
-- ranking every Chef by how many dishes they've cooked.
CREATE OR REPLACE VIEW leaderboard AS
    SELECT
        RANK() OVER (ORDER BY points DESC) AS `rank`,
        user,
        points,
        datetime_stamp AS last_update
    FROM socket_data
    ORDER BY points DESC;

-- ── 6. OPENING THE DINER (SEED DATA) ────────────────────────────
-- Day 1 setup: 3 Chefs clock in for their shift, each starting
-- with 0 dishes cooked.
INSERT INTO socket_data (user, points, datetime_stamp) VALUES
    ('c_server_user',        0, NOW()),   -- Chef C (Teammate 1's station)
    ('python_server_user_1', 0, NOW()),   -- Chef Py-1 (Teammate 2's station)
    ('python_server_user_2', 0, NOW())    -- Chef Py-2 (Nadzmi's station)
ON DUPLICATE KEY UPDATE user = user;

-- ── 7. OPENING CHECKS (VERIFY) ──────────────────────────────────
SELECT '=== Diner Tables Ready ===' AS info;
SHOW TABLES;

SELECT '=== Order Book (Live Status) ===' AS info;
SELECT * FROM socket_data;

SELECT '=== Top Chef Board ===' AS info;
SELECT * FROM leaderboard;
