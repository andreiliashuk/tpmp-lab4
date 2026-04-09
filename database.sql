PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS TRAWLERS (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    displacement REAL CHECK(displacement > 0),
    build_date TEXT NOT NULL,
    photo BLOB
);

CREATE TABLE IF NOT EXISTS CREW (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    surname TEXT NOT NULL,
    position TEXT NOT NULL,
    hire_date TEXT NOT NULL,
    birth_year INTEGER CHECK(birth_year BETWEEN 1940 AND 2005),
    trawler_id INTEGER,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    role TEXT CHECK(role IN ('admin', 'crew')) NOT NULL DEFAULT 'crew',
    FOREIGN KEY (trawler_id) REFERENCES TRAWLERS(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS BANKS (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS TRIPS (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    trawler_id INTEGER NOT NULL,
    bank_id INTEGER NOT NULL,
    departure_date TEXT NOT NULL,
    return_date TEXT NOT NULL,
    FOREIGN KEY (trawler_id) REFERENCES TRAWLERS(id),
    FOREIGN KEY (bank_id) REFERENCES BANKS(id),
    CHECK(return_date >= departure_date)
);

CREATE TABLE IF NOT EXISTS CATCH (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    trip_id INTEGER NOT NULL,
    fish_name TEXT NOT NULL,
    quality TEXT CHECK(quality IN ('high', 'medium', 'low')) NOT NULL,
    quantity_kg REAL CHECK(quantity_kg >= 0),
    FOREIGN KEY (trip_id) REFERENCES TRIPS(id)
);

CREATE TABLE IF NOT EXISTS STATS (
    trawler_id INTEGER PRIMARY KEY,
    total_catch_kg REAL DEFAULT 0,
    FOREIGN KEY (trawler_id) REFERENCES TRAWLERS(id)
);

CREATE TABLE IF NOT EXISTS BONUSES (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    crew_id INTEGER NOT NULL,
    period_start TEXT NOT NULL,
    period_end TEXT NOT NULL,
    amount REAL NOT NULL,
    FOREIGN KEY (crew_id) REFERENCES CREW(id)
);

CREATE TRIGGER IF NOT EXISTS update_stats_on_catch
AFTER INSERT ON CATCH
BEGIN
    UPDATE STATS
    SET total_catch_kg = (
        SELECT COALESCE(SUM(c2.quantity_kg), 0)
        FROM TRIPS t2
        JOIN CATCH c2 ON c2.trip_id = t2.id
        WHERE t2.trawler_id = (SELECT trawler_id FROM TRIPS WHERE id = NEW.trip_id)
    )
    WHERE trawler_id = (SELECT trawler_id FROM TRIPS WHERE id = NEW.trip_id);
END;
