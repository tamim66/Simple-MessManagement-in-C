#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void print_in_box(const char* text) {
    int len = strlen(text);
    // Adjust the box width based on the length of the text, adding some padding
    int box_width = len + 2;

    printf("+");
    for (int i = 0; i < box_width; i++) {
        printf("-");
    }
    printf("+\n");

    printf("| %s |\n", text);

    printf("+");
    for (int i = 0; i < box_width; i++) {
        printf("-");
    }
    printf("+\n");
}


void add_user(MYSQL* conn, const char* name) {
    char query[256];
    snprintf(query, sizeof(query), "INSERT INTO members (name) VALUES ('%s')", name);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT failed. Error: %s\n", mysql_error(conn));
        return;
    }

    // Prepare success message
    char success_message[256];
    snprintf(success_message, sizeof(success_message), "User '%s' added successfully!", name);

    // Print the success message in a box
    print_in_box(success_message);
}

void show_users(MYSQL* conn) {
    const char* query = "SELECT id, name FROM members";
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to fetch users. Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        fprintf(stderr, "Failed to store result. Error: %s\n", mysql_error(conn));
        return;
    }

    int num_rows = mysql_num_rows(result);
    if (num_rows == 0) {
        print_in_box("No users found.");
        mysql_free_result(result);
        return;
    }

    // Prepare the header
    printf("+-----------------------------+\n");
    printf("| ID   | Name                 |\n");
    printf("+-----------------------------+\n");

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        int id = atoi(row[0]);
        const char* name = row[1];
        printf("| %-4d | %-20s |\n", id, name);
    }

    printf("+-----------------------------+\n");

    mysql_free_result(result);
}

void log_meals(MYSQL* conn, int member_id, const char* meal_date, float meal_count, float meal_rate) {
    char query[512];
    snprintf(query, sizeof(query),
        "INSERT INTO meals (member_id, meal_date, meal_count, meal_rate) "
        "VALUES (%d, '%s', %f, %f)",
        member_id, meal_date, meal_count, meal_rate);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT failed. Error: %s\n", mysql_error(conn));
        return;
    }

    char message[150];
    snprintf(message, sizeof(message),
        "Meal logged for member ID %d on %s! Count: %.0f, Rate: %.0f",
        member_id, meal_date, meal_count, meal_rate);
    print_in_box(message);
}


void add_payment(MYSQL* conn, int member_id, float amount) {
    // Query to insert the payment into the expenses table without the payment date
    char query[256];
    snprintf(query, sizeof(query), "INSERT INTO expenses (amount, description) "
        "VALUES (%.2f, 'Payment by member ID %d')",
        amount, member_id);

    // Execute the query to insert the payment data
    if (mysql_query(conn, query)) {
        fprintf(stderr, "INSERT failed. Error: %s\n", mysql_error(conn));
        return;
    }
}



void view_total_expenses(MYSQL* conn) {
    char query[1024];

    // Query to get meal data
    snprintf(query, sizeof(query),
        "SELECT m.meal_date, mem.name, SUM(m.meal_count * m.meal_rate) AS meal_expenses, "
        "SUM(m.meal_count) AS total_meal_count "
        "FROM meals m "
        "INNER JOIN members mem ON m.member_id = mem.id "
        "GROUP BY m.meal_date, mem.id "
        "ORDER BY m.meal_date");

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT failed. Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed. Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_ROW row;
    double total_expenses = 0.0;
    char current_date[20] = "";

    printf("Total Expenses Summary:\n\n");

    while ((row = mysql_fetch_row(res)) != NULL) {
        const char* meal_date = row[0];
        const char* name = row[1];
        double meal_expenses = atof(row[2]);  // Convert meal_expenses to double
        int total_meal_count = atoi(row[3]);  // Convert meal_count to int

        // If the date changes, print the subtotal for the previous date
        if (strcmp(current_date, meal_date) != 0) {
            if (strlen(current_date) > 0) {
                printf("| %-10s | Subtotal = %.0fTk\n\n", current_date, total_expenses);
            }

            // Use strcpy_s to safely copy the current date
            strcpy_s(current_date, sizeof(current_date), meal_date);
            total_expenses = 0.0;
        }

        // Print meal data for each member
        printf("| %-10s | %-15s | meal - %-2d | = %.0fTk\n", meal_date, name, total_meal_count, meal_expenses);

        // Accumulate the total expenses for the current day
        total_expenses += meal_expenses;
    }

    // Print the subtotal for the last date
    if (strlen(current_date) > 0) {
        printf("| %-10s | Subtotal = %.0fTk\n\n", current_date, total_expenses);
    }

    mysql_free_result(res);
}



// Function to get total paid from the expenses table
double get_total_paid(MYSQL* conn, int member_id) {
    char query[1024];
    snprintf(query, sizeof(query),
        "SELECT SUM(amount) FROM expenses WHERE description LIKE CONCAT('Payment by member ID ', %d)", member_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT failed. Error: %s\n", mysql_error(conn));
        return 0.0;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed. Error: %s\n", mysql_error(conn));
        return 0.0;
    }

    MYSQL_ROW row;
    double total_paid = 0.0;
    if ((row = mysql_fetch_row(res)) != NULL) {
        total_paid = atof(row[0]);
    }

    mysql_free_result(res);
    return total_paid;
}

// Function to get meal cost and meal count from the meals table
double get_meal_cost(MYSQL* conn, int member_id, int* total_meal_count) {
    char query[1024];
    snprintf(query, sizeof(query),
        "SELECT SUM(meal_count * meal_rate), SUM(meal_count) FROM meals WHERE member_id = %d", member_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT failed. Error: %s\n", mysql_error(conn));
        return 0.0;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed. Error: %s\n", mysql_error(conn));
        return 0.0;
    }

    MYSQL_ROW row;
    double meal_cost = 0.0;
    if ((row = mysql_fetch_row(res)) != NULL) {
        meal_cost = atof(row[0]);  // Total meal cost
        *total_meal_count = atoi(row[1]);  // Total meal count
    }

    mysql_free_result(res);
    return meal_cost;
}

// View user summary function
void view_user_summary(MYSQL* conn) {
    char query[512];
    snprintf(query, sizeof(query), "SELECT id, name FROM members");

    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT failed. Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "mysql_store_result() failed. Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_ROW row;
    printf("+-------------------+-------------------+-------------------+-----------------+\n");
    printf("| Name              | Meal Count        | Payment Amount    | Due Amount      |\n");
    printf("+-------------------+-------------------+-------------------+-----------------+\n");

    while ((row = mysql_fetch_row(res)) != NULL) {
        int member_id = atoi(row[0]);
        const char* name = row[1];

        // Get total paid from expenses
        double total_paid = get_total_paid(conn, member_id);

        // Get meal cost and meal count from meals
        int total_meal_count = 0;
        double meal_cost = get_meal_cost(conn, member_id, &total_meal_count);

        // Calculate due amount
        double due_amount = meal_cost - total_paid;

        // Print user info
        printf("| %-17s | %-17d | %-17.0f | %-15.0f |\n", name, total_meal_count, total_paid, due_amount);
    }

    printf("+-------------------+-------------------+-----------------+-------------------+\n");

    mysql_free_result(res);
}

void reset_auto_increment(MYSQL* conn) {
    char query[256];
    snprintf(query, sizeof(query), "ALTER TABLE members AUTO_INCREMENT = 1");

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to reset AUTO_INCREMENT. Error: %s\n", mysql_error(conn));
        return;
    }

    printf("Auto-increment counter reset successfully.");
}

void delete_user(MYSQL* conn, int member_id) {
    char query[256];

    // Check if the user has meals recorded
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM meals WHERE member_id = %d", member_id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT failed. Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    int meal_count = atoi(row[0]);
    mysql_free_result(res);

    // If there are meals, delete them first
    if (meal_count > 0) {
        snprintf(query, sizeof(query), "DELETE FROM meals WHERE member_id = %d", member_id);
        if (mysql_query(conn, query)) {
            fprintf(stderr, "Delete meals failed. Error: %s\n", mysql_error(conn));
            return;
        }
        printf("Meals data for Member ID %d deleted.\n", member_id);
    }

    // Check if the user has expenses recorded
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM expenses WHERE description LIKE CONCAT('Payment by member ID ', %d)", member_id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "SELECT failed. Error: %s\n", mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    int expense_count = atoi(row[0]);
    mysql_free_result(res);

    // If there are expenses, delete them first
    if (expense_count > 0) {
        snprintf(query, sizeof(query), "DELETE FROM expenses WHERE description LIKE CONCAT('Payment by member ID ', %d)", member_id);
        if (mysql_query(conn, query)) {
            fprintf(stderr, "Delete expenses failed. Error: %s\n", mysql_error(conn));
            return;
        }
        printf("Expenses data for Member ID %d deleted.\n", member_id);
    }

    // Finally, delete the member from the members table
    snprintf(query, sizeof(query), "DELETE FROM members WHERE id = %d", member_id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Delete user failed. Error: %s\n", mysql_error(conn));
        return;
    }

    printf("Member ID %d deleted successfully.\n", member_id);

    // Reset auto-increment after deletion
    reset_auto_increment(conn);
}

void menu(MYSQL* conn) {
    int choice;
    int member_id;
    float meal_count, amount, meal_rate;
    char name[100], meal_date[20];

    while (1) {
        print_in_box("====== Mess Management System Menu ======");
        printf("1. Add User\n");
        printf("2. Log Meals\n");
        printf("3. Add Payment\n");
        printf("4. View Total Expenses\n");
        printf("5. View User Summary\n");
        printf("6. Delete User\n");
        printf("0. Exit\n");
        printf("\n");
        printf("Enter your choice: ");
        scanf_s("%d", &choice);

        switch (choice) {
        case 1:
            show_users(conn);
            printf("\n");
            printf("Enter new user name: ");
            scanf_s("%99s", name, (unsigned int)sizeof(name));
            add_user(conn, name);
            break;
        case 2:
            show_users(conn);
            printf("Enter member ID: ");
            scanf_s("%d", &member_id);
            printf("Enter meal date (YYYY-MM-DD): ");
            scanf_s("%19s", meal_date, (unsigned int)sizeof(meal_date));
            printf("Enter meal count: ");
            scanf_s("%f", &meal_count);
            printf("Enter meal rate: ");
            scanf_s("%f", &meal_rate);
            log_meals(conn, member_id, meal_date, meal_count, meal_rate);
            break;
        case 3:
            show_users(conn);  // Show all users
            printf("\nEnter member ID: ");
            scanf_s("%d", &member_id);  // Read member ID
            printf("Enter payment amount: ");
            scanf_s("%f", &amount);  // Read payment amount
            add_payment(conn, member_id, amount);  // Add the payment to the expenses table
            printf("\nPayment added successfully.\n");
            break;
        case 4:
            printf("\n");
            view_total_expenses(conn);
            printf("\n");
            break;
        case 5:
            printf("\n");
            view_user_summary(conn);
            printf("\n");
            break;
        case 6:
            show_users(conn);
            printf("\n");
            printf("Enter the member ID to delete: ");
            scanf_s("%d", &member_id);
            delete_user(conn, member_id);
            printf("\n");
            break;
        case 0:
            print_in_box("Exiting...");
            mysql_close(conn);
            exit(0);
        default:
            printf("Invalid choice. Please try again.\n");
        }
    }
}

int main() {
    MYSQL* conn;
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return EXIT_FAILURE;
    }

    if (mysql_real_connect(conn, "localhost", "root", "", "mess", 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed. Error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    printf("\nSuccessfully connected to the 'mess' database!\n");

    menu(conn);

    return EXIT_SUCCESS;
}
