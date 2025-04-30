import employee_db

# Crear instancia de la clase EmployeeDB
emp_id = 9

db = employee_db.EmployeeDB()

# DELETE: Eliminar un empleado

print(f"\nEliminando empleado con ID {emp_id}...")
"""
try:
    db.delete_employee(emp_id)
except RuntimeError as e:
    print(f"Error: {e}")
"""

# CREATE: Insertar un nuevo empleado
print("Creando un nuevo empleado...")
db.create_employee("Osvaldo", "Rios", "osvaldo.rios1@hey.in", 22000.0)

# READ: Obtener todos los empleados
print("\nObteniendo todos los empleados:")
employees = db.read_all_employees()
for emp in employees:
    print({k: v for k, v in emp.items()})

# READ: Obtener un empleado por ID
print(f"\nObteniendo empleado con ID {emp_id}:")
emp = db.read_employee_by_id(emp_id)
print({k: v for k, v in emp.items()})

# UPDATE: Actualizar un empleado
print(f"\nActualizando empleado con ID {emp_id}...")
db.update_employee(emp_id, "Osvaldo", "Rios Zambrano", "osvaldo.rios1@hey.inc", 23000.0)
emp = db.read_employee_by_id(emp_id)
print({k: v for k, v in emp.items()})

# DELETE: Eliminar un empleado
print(f"\nEliminando empleado con ID {emp_id}...")
db.delete_employee(emp_id)

try:
    db.read_employee_by_id(emp_id)
except RuntimeError as e:
    print(f"Error: {e}")
