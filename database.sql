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

INSERT OR IGNORE INTO TRAWLERS VALUES (1, 'Тайфун', 5000.0, '2010-05-15', NULL);
INSERT OR IGNORE INTO TRAWLERS VALUES (2, 'Шторм', 4500.0, '2012-08-20', NULL);

INSERT OR IGNORE INTO CREW VALUES (1, 'Иванов', 'captain', '2015-01-10', 1960, 1, 'cap_ivanov', '123', 'crew');
INSERT OR IGNORE INTO CREW VALUES (2, 'Петров', 'sailor', '2018-03-15', 1995, 1, 'sail_petrov', '123', 'crew');
INSERT OR IGNORE INTO CREW VALUES (3, 'Сидоров', 'captain', '2014-06-01', 1963, 2, 'cap_sid', '123', 'crew');

INSERT OR IGNORE INTO BANKS VALUES (1, 'Северная банка');
INSERT OR IGNORE INTO BANKS VALUES (2, 'Южная банка');

INSERT OR IGNORE INTO TRIPS VALUES (1, 1, 1, '2023-06-01', '2023-06-15');
INSERT OR IGNORE INTO TRIPS VALUES (2, 1, 2, '2023-07-10', '2023-07-25');
INSERT OR IGNORE INTO TRIPS VALUES (3, 2, 1, '2023-08-01', '2023-08-20');

INSERT OR IGNORE INTO CATCH VALUES (1, 1, 'Треска', 'high', 600.0);
INSERT OR IGNORE INTO CATCH VALUES (2, 1, 'Сельдь', 'low', 150.0);
INSERT OR IGNORE INTO CATCH VALUES (3, 2, 'Минтай', 'high', 800.0);
INSERT OR IGNORE INTO CATCH VALUES (4, 3, 'Треска', 'low', 300.0);
INSERT OR IGNORE INTO CATCH VALUES (5, 3, 'Сельдь', 'medium', 200.0);

INSERT OR IGNORE INTO STATS VALUES (1, 0), (2, 0);
