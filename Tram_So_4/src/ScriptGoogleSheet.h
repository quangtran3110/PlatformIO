function doGet(e) {
  Logger.log(JSON.stringify(e));
  var result = 'Ok';
  if (e.parameter == 'undefined') {
    result = 'No Parameters';
  } else {
    var sheet_id = '1mtDdVLuhiJs9rP9f5p50mvjW2IsiOfP5hdFlSkaQvyY'; // Spreadsheet ID.
    var sheet_name = "Tram_4";                                     // Sheet Name in Google Sheets.

    var sheet_open = SpreadsheetApp.openById(sheet_id);
    var sheet_target = sheet_open.getSheetByName(sheet_name);

    var newRow = sheet_target.getLastRow();

    var rowDataLog = [];

    var Curr_Date = Utilities.formatDate(new Date(), "Asia/Jakarta", 'dd/MM/yyyy');
    rowDataLog[0] = Curr_Date; // Date will be written in column A (in the "DHT11 Sensor Data Logger" section).

    var Curr_Time = Utilities.formatDate(new Date(), "Asia/Jakarta", 'HH:mm:ss');
    rowDataLog[1] = Curr_Time; // Time will be written in column B (in the "DHT11 Sensor Data Logger" section).

    var sts_val = '';

    for (var param in e.parameter) {
      Logger.log('In for loop, param=' + param);
      var value = stripQuotes(e.parameter[param]);
      Logger.log(param + ':' + e.parameter[param]);
      switch (param) {
      case 'sts':
        sts_val = value;
        break;

      case 'AL':
        rowDataLog[2] = value;
        result += ', Ap Luc Written';
        break;

      case 'AG1':
        rowDataLog[3] = value;
        result += ', Ampe Gieng Written';
        break;

      case 'AB1':
        rowDataLog[4] = value;
        result += ', Ampe Bom 1 Written';
        break;

      case 'AB2':
        rowDataLog[5] = value;
        result += ', Ampe Bom 2 Written';
        break;

      case 'ANK':
        rowDataLog[6] = value;
        result += ', Ampe Nen Khi Written';
        break;

      case 'MN':
        rowDataLog[7] = value;
        result += ', Muc Nuoc Written';
        break;

      default:
        result += ", unsupported parameter";
      }
    }

    // Conditions for writing data received from ESP32 to Google Sheets.
    if (sts_val == 'write') {
      // Writes data to the "DHT11 Sensor Data Logger" section.
      Logger.log(JSON.stringify(rowDataLog));
      // Insert a new row above the existing data.
      sheet_target.insertRows(2);
      var newRangeDataLog = sheet_target.getRange(2, 1, 1, rowDataLog.length);
      newRangeDataLog.setValues([rowDataLog]);
      // Định dạng các cột C đến G là dạng SỐ với 2 chữ số thập phân (ví dụ: 22.08)
      sheet_target.getRange("C:H").setNumberFormat('0.00');
      maxRowData(10000);
      return ContentService.createTextOutput(result);
    }

    // Conditions for sending data to ESP32 when ESP32 reads data from Google Sheets.
    if (sts_val == 'read') {
      // Use the line of code below if you want ESP32 to read data from columns I3 to O3 (Date,Time,Sensor Reading Status,Temperature,Humidity,Switch 1, Switch 2).
      // var all_Data = sheet_target.getRange('I3:O3').getDisplayValues();

      // Use the line of code below if you want ESP32 to read data from columns K3 to O3 (Sensor Reading Status,Temperature,Humidity,Switch 1, Switch 2).
      // var all_Data = sheet_target.getRange('K3:O3').getValues();
      return ContentService.createTextOutput(all_Data);
    }
  }
}
function maxRowData(allRowsAfter){
    const sheet = SpreadsheetApp.getActiveSpreadsheet()
                      .getSheetByName('Tram_4')

                          sheet.getRange(allRowsAfter + 1, 1, sheet.getLastRow() - allRowsAfter, sheet.getLastColumn())
                      .clearContent()

} function stripQuotes(value) {
  return value.replace(/ ^["']|['"] $ / g, "");
}
//________________