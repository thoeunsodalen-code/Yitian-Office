// =====================================================
// SMART OFFICE - GOOGLE APPS SCRIPT
// =====================================================

const SPREADSHEET_ID = "1ZL8o-j3Z2-yb3GRY_APgwp8_92V5hII1hU28yWwPriE";
const SHEET_NAME = "Attendance";
const TIMEZONE = "Asia/Phnom_Penh";


// =====================================================
// GET REQUEST FROM ESP8266
// =====================================================

function doGet(e) {

  try {

    // -------------------------------------------------
    // Check whether parameters were received
    // -------------------------------------------------

    if (!e || !e.parameter) {

      return ContentService
        .createTextOutput("ERROR: No parameters received");

    }


    // -------------------------------------------------
    // Receive parameters
    // -------------------------------------------------

    const firstName =
      (e.parameter.firstName || "").trim();

    const lastName =
      (e.parameter.lastName || "").trim();

    const DeptName =
      (e.parameter.DeptName || "").trim();

    const RoleName =
      (e.parameter.RoleName || "").trim();

    const uid =
      (e.parameter.uid || "").trim();


    // -------------------------------------------------
    // Validate required information
    // -------------------------------------------------

    if (
      firstName === "" ||
      lastName === "" ||
      uid === ""
    ) {

      return ContentService
        .createTextOutput(
          "ERROR: Missing employee information"
        );

    }


    // -------------------------------------------------
    // Open spreadsheet
    // -------------------------------------------------

    const ss =
      SpreadsheetApp.openById(
        SPREADSHEET_ID
      );

    const sheet =
      ss.getSheetByName(
        SHEET_NAME
      );


    if (!sheet) {

      return ContentService
        .createTextOutput(
          "ERROR: Sheet not found"
        );

    }


    // -------------------------------------------------
    // Get current date/time
    // -------------------------------------------------

    const now = new Date();

    const date =
      Utilities.formatDate(
        now,
        TIMEZONE,
        "dd/MM/yyyy"
      );

    const time =
      Utilities.formatDate(
        now,
        TIMEZONE,
        "HH:mm:ss"
      );


    const timestamp =
      Utilities.formatDate(
        now,
        TIMEZONE,
        "dd/MM/yyyy HH:mm:ss"
      );


    // -------------------------------------------------
    // Determine previous attendance state
    // -------------------------------------------------

    let lastAttendanceType = "";

    const lastRow =
      sheet.getLastRow();


    if (lastRow >= 2) {

      const data =
        sheet
          .getRange(
            2,
            1,
            lastRow - 1,
            9
          )
          .getValues();


      // Search from newest record to oldest
      for (
        let i = data.length - 1;
        i >= 0;
        i--
      ) {

        const rowUID =
          String(data[i][8] || "").trim();

        /*
          UID is not stored in the visible
          Timestamp column.

          Therefore we use a hidden/helper
          column later.

        */

      }

    }


    // -------------------------------------------------
    // Determine IN / OUT using today's records
    // -------------------------------------------------

    let latestType = "";

    if (lastRow >= 2) {

      const data =
        sheet
          .getRange(
            2,
            1,
            lastRow - 1,
            10
          )
          .getValues();


      for (
        let i = data.length - 1;
        i >= 0;
        i--
      ) {

        const rowFirstName =
          String(data[i][0] || "").trim();

        const rowLastName =
          String(data[i][1] || "").trim();

        const rowDate =
          String(data[i][6] || "").trim();

        const rowUID =
          String(data[i][9] || "").trim();


        if (
          rowFirstName === firstName &&
          rowLastName === lastName &&
          rowDate === date &&
          rowUID === uid
        ) {

          latestType =
            String(data[i][5] || "").trim();

          break;

        }

      }

    }


    // -------------------------------------------------
    // Decide IN / OUT
    // -------------------------------------------------

    let attendanceType;

    if (latestType === "IN") {

      attendanceType = "OUT";

    } else {

      attendanceType = "IN";

    }


    // -------------------------------------------------
    // Employee status
    // -------------------------------------------------

    const status = "Present";


    // -------------------------------------------------
    // Save attendance
    //
    // Column J = RFID UID
    // This is used internally to uniquely identify
    // the employee.
    // -------------------------------------------------

    sheet.appendRow([
      firstName,
      lastName,
      DeptName,
      RoleName,
      status,
      attendanceType,
      date,
      time,
      timestamp,
      uid
    ]);


    // -------------------------------------------------
    // Return response to ESP8266
    // -------------------------------------------------

    return ContentService
      .createTextOutput(
        attendanceType
      );

  }


  catch (error) {

    return ContentService
      .createTextOutput(
        "ERROR: " + error.message
      );

  }

}
