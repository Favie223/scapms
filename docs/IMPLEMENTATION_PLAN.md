# SCAPMS — Implementation Plan

Smart Clinic Appointment & Patient Management System (COS202 Programming II lab assessment).
Stack: **C++17 + Qt 6 Widgets + CMake** (already chosen in `CMakeLists.txt`).

---

## 1. Current state of the codebase

| File | What it is | Status |
|---|---|---|
| `CMakeLists.txt` | Qt6 ≥ 6.8 Widgets, C++17, one executable from `main.cpp` | Keep. Needs every new source added, `CMAKE_AUTOMOC` (already set by `qt_standard_project_setup()`) |
| `main.cpp` | 94 lines: one `QWidget` with a title label and 5 buttons that each pop a placeholder `QMessageBox` | Throwaway prototype. Gets replaced by a 10-line `main` that opens `gui::MainWindow` |
| `README.md` | Saved as **UTF-16 LE with CRLF**, just `# scapms` | Re-save as UTF-8 and give it build instructions |
| `.idea/` | CLion project files | Fine to keep (add `cmake-build-*/` to `.gitignore`) |

**Nothing in the requirements is implemented yet.** No model classes, no data structures, no algorithms, no persistence. The "Doctor Information" button doesn't map to any requirement and can go.

**Environment note:** `cmake` and Qt 6 aren't on this Mac's PATH, and I didn't find CLion in `/Applications`. Before coding starts, install a toolchain: `brew install cmake qt` (Qt 6.8+), or use the Qt Online Installer and point CLion at it.

---

## 2. Target folder structure

Follows the structure suggested in the brief (pp. 2–4), plus the AdminPanel, SearchPanel and Reports panel it names.

```
scapms/
├── CMakeLists.txt
├── README.md
├── main.cpp
├── data/                         # sample data shipped with the app
│   ├── patients.csv
│   └── appointments.csv
├── docs/
│   ├── IMPLEMENTATION_PLAN.md
│   ├── uml.puml                  # class diagram source
│   └── report.md                 # 2–3 page report draft
├── model/                        namespace scapms::model
│   ├── PatientRecord.h/.cpp      # abstract base
│   ├── Appointable.h             # interface (pure virtual only)
│   ├── GeneralPatient.h/.cpp
│   ├── EmergencyPatient.h/.cpp
│   ├── FollowUpPatient.h/.cpp
│   ├── Appointment.h/.cpp
│   └── ClinicDatabase.h/.cpp     # composition: owns patients + appointments
├── controller/                   namespace scapms::controller
│   ├── ClinicManager.h/.cpp      # facade: CRUD, queue, undo stack, cache, reports
│   ├── AppointmentController.h/.cpp
│   ├── SearchEngine.h/.cpp       # linear, binary, recursive search
│   ├── SortEngine.h/.cpp         # selection, insertion, merge, quick sort
│   ├── UndoManager.h/.cpp        # std::stack<AdminOperation>
│   ├── RecentCache.h/.cpp        # fixed-size array cache
│   └── ReportGenerator.h/.cpp
├── gui/                          namespace scapms::gui
│   ├── MainWindow.h/.cpp         # QMainWindow, menus, tabs, status bar, timers
│   ├── DashboardPanel.h/.cpp     # stats + recent patients
│   ├── PatientPanel.h/.cpp       # table + add/edit/delete
│   ├── PatientDialog.h/.cpp      # add/edit form with dynamic fields
│   ├── AppointmentPanel.h/.cpp
│   ├── QueuePanel.h/.cpp
│   ├── SearchPanel.h/.cpp        # search + sort with algorithm combo boxes
│   ├── AdminPanel.h/.cpp         # undo history, undo button
│   ├── ReportsPanel.h/.cpp
│   └── PatientTableModel.h/.cpp  # QAbstractTableModel + colour coding
└── utils/                        namespace scapms::utils
    ├── IDGenerator.h/.cpp
    ├── FileHandler.h/.cpp        # CSV save/load
    ├── Validation.h/.cpp
    └── Exceptions.h              # custom exception hierarchy
```

**Layering rule (what the marker checks under §2):** `model` depends on nothing in Qt, `controller` depends only on `model` + `utils`, and `gui` is the only layer that includes Qt Widgets. Keep `model`/`controller` on plain `std::string` and convert to `QString` at the GUI boundary. That shows the separation clearly and keeps the algorithms obviously hand-written.

---

## 3. Data model

### 3.1 Class diagram (text form; becomes `docs/uml.puml`)

```
             «interface»                      «abstract»
             Appointable                      PatientRecord
   + scheduleAppointment(Appointment&)=0     # id: string
   + cancelAppointment(apptId)=0             # name: string
   + appointmentIds(): vector<string>=0      # age: int
             ▲                               # contact: ContactInfo
             │ implements                    + category(): string = 0        ◄─ pure virtual
             │                               + priority(): int = 0           ◄─ pure virtual
             │                               + describe(): string = 0        ◄─ pure virtual
             │                               + toCsvExtra(): string = 0
             │                               + clone(): unique_ptr<PatientRecord> = 0
             │                               + virtual ~PatientRecord()
             │                                         ▲
     ┌───────┴──────────────┬──────────────────────────┼──────────────────────┐
     │                      │                          │                      │
 GeneralPatient       EmergencyPatient           FollowUpPatient
 - insuranceNo        - severity (1–5)            - previousVisitDate
 - primaryComplaint   - triageNotes               - referringDoctor
 priority()=Normal    - arrivalTime               - followUpReason
                      priority()=High (by sev.)   priority()=Scheduled

ClinicDatabase ◆── vector<unique_ptr<PatientRecord>>   (composition, owns)
               ◆── vector<Appointment>
Appointment ──> patientId   (association; a patient has 1..* appointments)
```

All three subclasses inherit `PatientRecord` and `Appointable` (multiple inheritance from an interface).

### 3.2 Fields

**ContactInfo** (struct): `phone`, `email`, `address`.

**PatientRecord** (abstract):
| Field | Type | Rules |
|---|---|---|
| `id` | `std::string` | `P0001`…; unique; from `IDGenerator` |
| `name` | `std::string` | non-empty, letters/spaces/`'-` |
| `age` | `int` | 0–120 → else `InvalidAgeException` |
| `contact` | `ContactInfo` | phone digits/+, length 7–15 |
| `appointmentIds` | `std::vector<std::string>` | kept in the base so all types share it |
| `registeredOn` | `std::string` (ISO date) | |

**Subclass behaviour** (what "behaviour appropriate to its patient type" means):
- `GeneralPatient`: `priority()` = 2. Can book any future slot.
- `EmergencyPatient`: `priority()` = 0 (highest). `scheduleAppointment` allows same-day/immediate slots and skips the usual lead-time rule. `describe()` shows severity.
- `FollowUpPatient`: `priority()` = 1. `scheduleAppointment` requires the date to be after `previousVisitDate`.

**Appointment**:
| Field | Type |
|---|---|
| `id` | `A0001`… |
| `patientId` | `std::string` |
| `dateTime` | `std::string` `"YYYY-MM-DD HH:MM"` (sorts lexicographically = chronologically) |
| `doctor` | `std::string` |
| `reason` | `std::string` |
| `status` | `enum class AppointmentStatus { Pending, Completed, Cancelled }` |

### 3.3 Persistence format (CSV, `utils::FileHandler`)

`patients.csv`
```
id,type,name,age,phone,email,address,registeredOn,extra1,extra2,extra3
P0001,General,Ada Obi,34,08031234567,ada@x.com,Hall 3,2026-11-01,INS-22,Headache,
P0002,Emergency,Tunde K,19,08021112222,,Block B,2026-11-10,4,Fracture,2026-11-10 09:12
```
`appointments.csv`
```
id,patientId,dateTime,doctor,reason,status
A0001,P0001,2026-11-14 10:30,Dr. Bello,Checkup,Pending
```
A factory in `FileHandler` reads `type` and builds the right subclass. Quote/escape commas in fields. If the file is missing or a row is malformed, throw `FileLoadException` with the line number.

---

## 4. Requirement → design mapping (checklist)

| # | Requirement | Where / how |
|---|---|---|
| 1a | Abstract `PatientRecord` with pure virtual | `model/PatientRecord.h`: `category()`, `priority()`, `describe()` = 0 |
| 1b | 3 subclasses with own behaviour | §3.2 |
| 1c | `Appointable` interface | `model/Appointable.h`, pure virtuals only, virtual dtor |
| 1d | Runtime polymorphism | `ReportGenerator::categoryDistribution(const vector<unique_ptr<PatientRecord>>&)` and `PatientTableModel::data()` call `p->category()`/`p->describe()` through base pointers; `ClinicManager::processAll(std::function<void(PatientRecord&)>)` |
| 1e | Encapsulation + composition | private fields + validated setters; `ClinicDatabase` owns patients & appointments |
| 1f | Headers/sources + namespaces | §2 |
| 3a | `std::vector` | `ClinicDatabase::patients_` |
| 3b | `std::queue` | `ClinicManager::waitingQueue_` (`std::queue<std::string>` of patient IDs; FIFO) |
| 3c | `std::stack` undo | `UndoManager`: `std::stack<AdminOperation>`; op = `{type, snapshot (clone), apptSnapshot}`. Covers DeletePatient, ModifyPatient, RemoveAppointment (and CancelAppointment) |
| 3d | Fixed array cache | `RecentCache`: `PatientRecord* slots_[10]`, MRU order. Shift on access, evict last. Clear the slot on delete so no pointer is left dangling |
| 4a | Linear search by ID | `SearchEngine::linearById` |
| 4b | Binary search | `SearchEngine::binaryById` (needs ID-sorted data) + `binaryByName` |
| 4c | Recursive search by name | `SearchEngine::recursiveByName(vec, key, index)`: recursive partial/case-insensitive match. Also `recursiveBinarySearch` |
| 4* | Choose by data state | `ClinicManager` tracks `sortedBy_` (None/Id/Name/Age/Date). `SearchEngine::smartSearch` picks binary if sorted on that key, else linear, and the GUI says which one it used |
| 5 | 2+ manual sorts, 2+ keys, combo box | `SortEngine` template on a comparator: Selection, Insertion, Merge, Quick. Keys: Name, ID, Age, next appointment date. No `std::sort` anywhere |
| 6a | Button events | Qt signals/slots on every panel |
| 6b | Menu events | File → New / Load / Save / Import… / Export… / Exit; Edit → Undo; Help → About |
| 6c | Text field events | `textChanged` on search box (live filter), ID field duplicate check, age field validation |
| 6d | Timer events | `QTimer`s: 60 s appointment-reminder check, 5 s dashboard refresh, 30 s waiting-time alert (patient waiting > N min) |
| 7 | Tabs/sections | `QTabWidget`: Dashboard, Patients, Appointments, Waiting Queue, Search & Sort, Admin, Reports |
| 7 | Components | `QTableView`, `QListWidget`, `QPushButton`, `QLineEdit`, `QComboBox`, `QLabel`, `QDialog`, `QMenuBar`, `QStatusBar` |
| 8 | 2+ layouts | `QVBoxLayout` (panels), `QHBoxLayout` (button rows), `QFormLayout` (patient/appointment forms), `QGridLayout` (dashboard stat cards), `QStackedLayout` (type-specific field group in PatientDialog) |
| 9 | 2+ advanced techniques | We do all of them: **(a)** colour-coded rows via `Qt::BackgroundRole` (Emergency red / General default / Follow-up blue; status colours in appointments), **(b)** dynamic fields per patient type in `PatientDialog`, **(c)** `QFileDialog` import/export, **(d)** timers, **(e)** validation dialogs, **(f)** shortcuts Ctrl+S/F/Q/Z/N/O, **(g)** tooltips |
| 10 | Exceptions + custom class | `utils/Exceptions.h`: `ClinicException : std::runtime_error` → `ValidationException`, `InvalidAgeException`, `DuplicateIdException`, `NotFoundException`, `InvalidAppointmentException`, `FileLoadException`, `InvalidSearchInputException`. Thrown in model/controller, caught in GUI slots → `QMessageBox::warning` |
| 11d | Persistence | CSV in `data/`; autoload on start, save on Ctrl+S and prompt on exit if there are unsaved changes |
| Reports | 3+ | (1) Most frequently attended (by Completed count), (2) patients with pending appts, (3) category distribution, (4) daily appointment summary for a picked date |

---

## 5. Key class interfaces (sketch)

```cpp
namespace scapms::model {
class PatientRecord {
public:
    PatientRecord(std::string id, std::string name, int age, ContactInfo c);
    virtual ~PatientRecord() = default;
    virtual std::string category() const = 0;
    virtual int priority() const = 0;
    virtual std::string describe() const = 0;
    virtual std::vector<std::string> extraFields() const = 0;   // for CSV/table
    virtual std::unique_ptr<PatientRecord> clone() const = 0;   // for undo snapshots
    const std::string& id() const; const std::string& name() const; int age() const;
    void setName(const std::string&); void setAge(int);         // validate + throw
    ...
protected:
    std::vector<std::string> appointmentIds_;
private:
    std::string id_, name_; int age_; ContactInfo contact_;
};

class Appointable {
public:
    virtual ~Appointable() = default;
    virtual void scheduleAppointment(const Appointment& a) = 0;
    virtual void cancelAppointment(const std::string& apptId) = 0;
    virtual bool canSchedule(const std::string& dateTime) const = 0;
};

class GeneralPatient : public PatientRecord, public Appointable { ... };
}

namespace scapms::controller {
class SortEngine {
public:
    enum class Algorithm { Selection, Insertion, Merge, Quick };
    enum class Key { Name, Id, Age, NextAppointment };
    template<class T, class Less> static void selectionSort(std::vector<T>&, Less);
    template<class T, class Less> static void insertionSort(std::vector<T>&, Less);
    template<class T, class Less> static void mergeSort(std::vector<T>&, Less);  // recursive
    template<class T, class Less> static void quickSort(std::vector<T>&, Less);  // recursive
    static void sort(std::vector<PatientRecord*>&, Algorithm, Key, const ClinicDatabase&);
};

class SearchEngine {
public:
    static int linearById(const std::vector<PatientRecord*>&, const std::string& id);
    static int binaryById(const std::vector<PatientRecord*>&, const std::string& id);
    static int binaryByName(const std::vector<PatientRecord*>&, const std::string& name);
    static void recursiveByName(const std::vector<PatientRecord*>&, const std::string& key,
                                size_t idx, std::vector<PatientRecord*>& out);
    static std::vector<PatientRecord*> byCategory(...);
};

class ClinicManager {                  // single entry point the GUI talks to
public:
    PatientRecord& addPatient(std::unique_ptr<PatientRecord>);
    void updatePatient(const std::string& id, const PatientEdit&);  // pushes undo
    void deletePatient(const std::string& id);                     // pushes undo
    void enqueue(const std::string& id); std::optional<std::string> callNext();
    bool undo(); std::vector<std::string> undoHistory() const;
    void save(const std::string& dir); void load(const std::string& dir);
    ...
private:
    model::ClinicDatabase db_;
    std::queue<std::string> waitingQueue_;
    UndoManager undo_;
    RecentCache cache_;
    SortKeyState sortedBy_;
};
}
```

Sorting works on a `std::vector<PatientRecord*>` *view* of the database. The database keeps ownership, and the binary search runs on that sorted view. Decision for the "sort by appointment date" key: sort by each patient's **earliest pending** appointment, and put patients with none last.

**Qt bridging:** `gui::ClinicNotifier : QObject` (or `MainWindow` itself) emits `dataChanged()` after each controller call, and all panels connect to it to refresh. Then the controller stays Qt-free while panels still sync through signals/slots.

---

## 6. Implementation phases

Each phase ends compiling and runnable. Commit after each one.

**Phase 0: Toolchain & skeleton (½ day)**
- Install CMake + Qt 6.8+, get the current `main.cpp` building.
- Make the folders. Re-save README as UTF-8. Add `.gitignore` (`cmake-build-*/`, `build/`, `.DS_Store`).
- Restructure CMake: `add_library(scapms_core STATIC model/* controller/* utils/*)` (no Qt) + `qt_add_executable(SmartClinicSystem main.cpp gui/*)`. Optionally add a tiny `scapms_tests` console exe for the algorithms.

**Phase 1: Model + utils (1 day)**
- `Exceptions.h`, `Validation`, `IDGenerator` (counter per prefix, reseeded from the max ID after load).
- `PatientRecord`, `Appointable`, three subclasses, `Appointment`, `ClinicDatabase`.
- Unit-check in the console test exe: create each type, call `describe()` via base pointer, confirm invalid age throws.

**Phase 2: Controller logic (1–1.5 days)**
- `SortEngine` (4 algorithms × 4 keys), `SearchEngine` (linear, binary ×2, recursive).
- `UndoManager` + `RecentCache`.
- `AppointmentController` (schedule/cancel/complete, uses `Appointable::canSchedule`).
- `ClinicManager` façade, queue, `ReportGenerator`.
- `FileHandler` CSV load/save + factory, and ship `data/*.csv` with ~20 patients / ~40 appointments.
- Test exe: sort 1000 random patients with each algorithm, check order. Search hits/misses. Undo round-trip. Save→load equality.

**Phase 3: GUI shell (1 day)**
- `MainWindow`: menu bar, `QTabWidget`, status bar ("23 patients · 4 waiting · sorted by Name (Merge)"), shortcuts, `closeEvent` unsaved-changes prompt.
- `PatientTableModel` with colour roles.
- `PatientPanel` + `PatientDialog` (QFormLayout + QStackedLayout dynamic fields, validators, tooltips).

**Phase 4: Remaining panels (1.5 days)**
- `AppointmentPanel`: patient combo, `QDateTimeEdit`, doctor, reason. Schedule/Cancel/Mark Completed. Status-coloured table.
- `QueuePanel`: add selected patient, "Call Next", list with join time, timer alert for long waits.
- `SearchPanel`: field combo (ID/Name/Category), algorithm combo (Auto/Linear/Binary/Recursive), live `textChanged`. Sort algorithm combo + key combo + SORT button, and show the time taken / comparison count.
- `AdminPanel`: undo history list from the stack + Undo button (Ctrl+Z).
- `ReportsPanel`: report combo + table/text output + Export.
- `DashboardPanel`: QGridLayout stat cards, today's appointments, recent-patients cache list. Refreshed by a 5 s timer.

**Phase 5: Hardening (½ day)**
- Wrap every slot body in `try { … } catch (const ClinicException& e) { warn } catch (const std::exception& e) { critical }`.
- Edge cases: empty DB search, binary search on unsorted data falls back to linear, delete a patient who's in the queue/cache (purge them from both), cancel an already-cancelled appointment, corrupt CSV row.
- Add a `grep` check that `std::sort`, `std::find`, `std::binary_search` and `std::lower_bound` don't appear in `controller/`.

**Phase 6: Deliverables (1 day)**
- UML diagram (PlantUML → PNG): inheritance, abstract, interface, composition (◆ ClinicDatabase), association (Appointment→Patient).
- Screenshots: dashboard, registration dialog, appointments, search/sort, queue, reports.
- 2–3 page report (§7).
- Single PDF: report + UML + screenshots + all source grouped under headers (Header files / Source files / GUI files / Data files). A small script can concatenate the sources into Markdown → PDF.

---

## 7. Report talking points to justify choices

- **Sorting.** *Insertion sort* is O(n) on nearly-sorted data, which fits this clinic: patients get added one at a time to an already-sorted list, and it's stable. *Merge sort* is O(n log n) guaranteed and stable, so it's the right pick for a full re-sort by a new key or for large imports. *Quick sort* is the fastest on average for random data, with an O(n²) worst case (we use median-of-three). *Selection sort* does the fewest swaps but is always O(n²), so it's only there for comparison. The GUI's comparison counter backs these claims with numbers.
- **Searching.** Linear search works on unsorted data and is O(n). Binary search is O(log n) but only valid after sorting on the same key, so `smartSearch` picks between them. The recursive name search shows the base case and recursive step, and handles partial matches.
- **Layouts.** `QFormLayout` for label/field forms gives aligned labels for free. `QGridLayout` for the dashboard cards. `QHBox`/`QVBox` for toolbars and panel stacking. `QStackedLayout` swaps the type-specific field set.
- **Data structures.** vector = random access for sorting and indexing. queue = FIFO fairness in the waiting room. stack = LIFO, so the newest operation is undone first. Fixed array = bounded memory and O(1) slot access for the MRU cache.
- **Challenges to write up:** dangling pointers in the cache and undo after deletes (solved with clone snapshots and purging the cache), keeping Qt out of the core, CSV escaping, and the UTF-16 README.

---

## 8. Open decisions (defaults chosen; change if you prefer)

1. **Persistence format.** CSV (simplest to show and hand-parse). JSON via `QJsonDocument` is an alternative, but it would pull Qt into `utils`.
2. **Which sorts to feature.** All four are implemented. The report focuses on Insertion + Merge.
3. **Undo scope.** Delete patient, modify patient, remove/cancel appointment. Adds aren't undoable (the brief only lists those three).
4. **Team split** (if this is group work): A = model + utils + persistence, B = controller algorithms + tests, C = GUI panels, D = report/UML/screenshots.
