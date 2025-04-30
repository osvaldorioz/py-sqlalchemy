#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <map>

namespace py = pybind11;

class EmployeeDB {
private:
    py::object engine;
    py::object Base;
    py::object Employee;
    py::object session;

    void initialize() {
        try {
            // Asegurar que el GIL esté adquirido
            py::gil_scoped_acquire acquire;

            // Importar módulos de SQLAlchemy
            std::cout << "Importando sqlalchemy..." << std::endl;
            py::module_ sqlalchemy = py::module::import("sqlalchemy");
            std::cout << "sqlalchemy importado." << std::endl;

            std::cout << "Importando sqlalchemy.orm..." << std::endl;
            py::module_ orm = py::module::import("sqlalchemy.orm");
            std::cout << "sqlalchemy.orm importado." << std::endl;

            // Crear engine para PostgreSQL
            std::string conn_str = "postgresql://hadoop:hadoop@localhost:5432/company_db";
            std::cout << "Conectando a: " << conn_str << std::endl;
            engine = sqlalchemy.attr("create_engine")(conn_str);
            std::cout << "Engine creado exitosamente." << std::endl;

            // Probar conexión explícitamente
            std::cout << "Probando conexión a la base de datos..." << std::endl;
            py::object conn = engine.attr("connect")();
            conn.attr("close")();
            std::cout << "Conexión probada exitosamente." << std::endl;

            // Crear base declarativa (SQLAlchemy 2.0)
            std::cout << "Creando base declarativa..." << std::endl;
            Base = orm.attr("declarative_base")();
            std::cout << "Base declarativa creada." << std::endl;

            // Definir el modelo Employee
            std::cout << "Definiendo modelo Employee..." << std::endl;
            py::dict class_dict;
            class_dict["__tablename__"] = "employees";
            class_dict["id"] = sqlalchemy.attr("Column")(sqlalchemy.attr("Integer"), py::arg("primary_key") = true);
            class_dict["first_name"] = sqlalchemy.attr("Column")(sqlalchemy.attr("String"));
            class_dict["last_name"] = sqlalchemy.attr("Column")(sqlalchemy.attr("String"));
            class_dict["email"] = sqlalchemy.attr("Column")(sqlalchemy.attr("String"), py::arg("unique") = true);
            class_dict["salary"] = sqlalchemy.attr("Column")(sqlalchemy.attr("Float"));

            // Crear la clase Employee usando Python puro para evitar problemas con py::class_
            py::object type_ = py::module::import("builtins").attr("type");
            Employee = type_("Employee", py::make_tuple(Base), class_dict);
            std::cout << "Modelo Employee definido." << std::endl;

            // Crear la tabla en la base de datos
            std::cout << "Creando tabla employees..." << std::endl;
            Base.attr("metadata").attr("create_all")(engine);
            std::cout << "Tabla creada." << std::endl;

            // Crear una sesión
            std::cout << "Creando sesión..." << std::endl;
            py::object Session = orm.attr("sessionmaker")(py::arg("bind") = engine);
            session = Session();
            std::cout << "Sesión creada exitosamente." << std::endl;
        } catch (const py::error_already_set& e) {
            std::cerr << "Error de Pybind11: " << e.what() << std::endl;
            throw std::runtime_error("Error inicializando SQLAlchemy: " + std::string(e.what()));
        } catch (const std::exception& e) {
            std::cerr << "Error estándar: " << e.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "Error desconocido en initialize()" << std::endl;
            throw std::runtime_error("Error desconocido en initialize()");
        }
    }

public:
    EmployeeDB() {
        initialize();
    }

    ~EmployeeDB() {
        try {
            py::gil_scoped_acquire acquire;
            if (session) {
                std::cout << "Cerrando sesión..." << std::endl;
                session.attr("close")();
            }
        } catch (const py::error_already_set& e) {
            std::cerr << "Error cerrando sesión: " << e.what() << std::endl;
        }
    }

    // CREATE: Insertar un nuevo empleado
    void create_employee(const std::string& first_name, const std::string& last_name,
                        const std::string& email, float salary) {
        try {
            py::gil_scoped_acquire acquire;
            py::object employee = Employee(
                py::arg("first_name") = first_name,
                py::arg("last_name") = last_name,
                py::arg("email") = email,
                py::arg("salary") = salary
            );
            session.attr("add")(employee);
            session.attr("commit")();
            std::cout << "Empleado creado: " << first_name << " " << last_name << std::endl;
        } catch (const py::error_already_set& e) {
            session.attr("rollback")();
            std::cerr << "Error creando empleado: " << e.what() << std::endl;
            throw std::runtime_error("Error creando empleado: " + std::string(e.what()));
        }
    }

    // READ: Obtener todos los empleados
    std::vector<std::map<std::string, py::object>> read_all_employees() {
        try {
            py::gil_scoped_acquire acquire;
            py::object employees = session.attr("query")(Employee).attr("all")();
            std::vector<std::map<std::string, py::object>> results;
            for (auto employee : employees) {
                std::map<std::string, py::object> emp_data;
                emp_data["id"] = employee.attr("id");
                emp_data["first_name"] = employee.attr("first_name");
                emp_data["last_name"] = employee.attr("last_name");
                emp_data["email"] = employee.attr("email");
                emp_data["salary"] = employee.attr("salary");
                results.push_back(emp_data);
            }
            std::cout << "Leídos " << results.size() << " empleados." << std::endl;
            return results;
        } catch (const py::error_already_set& e) {
            std::cerr << "Error leyendo empleados: " << e.what() << std::endl;
            throw std::runtime_error("Error leyendo empleados: " + std::string(e.what()));
        }
    }

    // READ: Obtener un empleado por ID
    std::map<std::string, py::object> read_employee_by_id(int id) {
        try {
            py::gil_scoped_acquire acquire;
            py::object employee = session.attr("query")(Employee)
                                    .attr("filter_by")(py::arg("id") = id)
                                    .attr("first")();
            if (employee.is_none()) {
                throw std::runtime_error("Empleado no encontrado con ID: " + std::to_string(id));
            }
            std::map<std::string, py::object> emp_data;
            emp_data["id"] = employee.attr("id");
            emp_data["first_name"] = employee.attr("first_name");
            emp_data["last_name"] = employee.attr("last_name");
            emp_data["email"] = employee.attr("email");
            emp_data["salary"] = employee.attr("salary");
            std::cout << "Empleado leído con ID: " << id << std::endl;
            return emp_data;
        } catch (const py::error_already_set& e) {
            std::cerr << "Error leyendo empleado por ID: " << e.what() << std::endl;
            throw std::runtime_error("Error leyendo empleado por ID: " + std::string(e.what()));
        }
    }

    // UPDATE: Actualizar un empleado por ID
    void update_employee(int id, const std::string& first_name, const std::string& last_name,
                        const std::string& email, float salary) {
        try {
            py::gil_scoped_acquire acquire;
            py::object employee = session.attr("query")(Employee)
                                    .attr("filter_by")(py::arg("id") = id)
                                    .attr("first")();
            if (employee.is_none()) {
                throw std::runtime_error("Empleado no encontrado con ID: " + std::to_string(id));
            }
            employee.attr("first_name") = first_name;
            employee.attr("last_name") = last_name;
            employee.attr("email") = email;
            employee.attr("salary") = salary;
            session.attr("commit")();
            std::cout << "Empleado actualizado con ID: " << id << std::endl;
        } catch (const py::error_already_set& e) {
            session.attr("rollback")();
            std::cerr << "Error actualizando empleado: " << e.what() << std::endl;
            throw std::runtime_error("Error actualizando empleado: " + std::string(e.what()));
        }
    }

    // DELETE: Eliminar un empleado por ID
    void delete_employee(int id) {
        try {
            py::gil_scoped_acquire acquire;
            py::object employee = session.attr("query")(Employee)
                                    .attr("filter_by")(py::arg("id") = id)
                                    .attr("first")();
            if (employee.is_none()) {
                throw std::runtime_error("Empleado no encontrado con ID: " + std::to_string(id));
            }
            session.attr("delete")(employee);
            session.attr("commit")();
            std::cout << "Empleado eliminado con ID: " << id << std::endl;
        } catch (const py::error_already_set& e) {
            session.attr("rollback")();
            std::cerr << "Error eliminando empleado: " << e.what() << std::endl;
            throw std::runtime_error("Error eliminando empleado: " + std::string(e.what()));
        }
    }
};

PYBIND11_MODULE(employee_db, m) {
    m.doc() = "Módulo para operaciones CRUD en PostgreSQL usando SQLAlchemy desde C++";
    py::class_<EmployeeDB>(m, "EmployeeDB")
        .def(py::init<>())
        .def("create_employee", &EmployeeDB::create_employee,
             "Crea un nuevo empleado",
             py::arg("first_name"), py::arg("last_name"), py::arg("email"), py::arg("salary"))
        .def("read_all_employees", &EmployeeDB::read_all_employees,
             "Obtiene todos los empleados")
        .def("read_employee_by_id", &EmployeeDB::read_employee_by_id,
             "Obtiene un empleado por ID", py::arg("id"))
        .def("update_employee", &EmployeeDB::update_employee,
             "Actualiza un empleado por ID",
             py::arg("id"), py::arg("first_name"), py::arg("last_name"), py::arg("email"), py::arg("salary"))
        .def("delete_employee", &EmployeeDB::delete_employee,
             "Elimina un empleado por ID", py::arg("id"));
}