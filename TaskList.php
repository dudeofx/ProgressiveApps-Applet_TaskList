<?php
   define("_NOP",         0x00);
   define("_GET_INDEX",   0x01);
   define("_GET_ITEM",    0x02);
   define("_INSERT",      0x03);
   define("_DELETE",      0x04);
   define("_UPDATE",      0x05);
   define("_INDEX",       0x06);
   define("_ITEM",        0x07);
   define("_NEW_ITEM",    0x08);
   define("_OK",          0x09);
   define("_ERROR",       0x0A);

   //--------------------------------------------------------------------
   function fail($err_msg) {
      error_log("error: ".$err_msg);
      http_response_code(500);
   }
   //--------------------------------------------------------------------
   function GetIndex($name) {
      $init_data = [
         0 => "This is a new task-list",
         1 => "Click on &#x1F4C4; to insert new item",
         2 => "Click on &#x1F4DD; to edit item",
         3 => "Set checkboxes and click &#x1f5d1;&#xfe0f; to delete items",
      ];

      $db = new SQLite3('TaskList.db');
      $db->busyTimeout(5000);

      $check = $db->query("SELECT name FROM sqlite_master WHERE name='".$name."'");
      if ($check->fetchArray() === false) {
         $db->exec("CREATE TABLE ".$name." (rowid INTEGER PRIMARY KEY, item TEXT NOT NULL, size INTEGER, start REAL, target REAL, modified REAL)");
         for ($i = 0; $i < count($init_data); $i++) {
            $db->exec("INSERT INTO ".$name." (item, size, start, target, modified) VALUES('".$init_data[$i]."', ".strlen($init_data[$i]).", julianday('now'), julianday('now'), julianday('now'))");
         }
      }

      $res = $db->query('SELECT * FROM '.$name);

      $count = 0;
      $offset = 0;
      while ($row = $res->fetchArray()) {
         $size = $row['size']+1;
         $index .= pack("LLL", $row['rowid'], $size, $offset);
         $data  .= pack("a".$size, $row['item']);
         $count++;
         $offset += $size;
      }

      $db->close();
      unset($db);

      $name_len = strlen($name);
      $name_raw = pack("a".$name_len, $name);

      $body_len = $count*3*4 + $offset;

      $header = pack("A4LLLLA4", "CpH1", _INDEX, $count, $name_len, $body_len, "z2C3");
      return $header.$name_raw.$index.$data;
   };
   //--------------------------------------------------------------------
   function GetItem($name, $item_id) {
      $db = new SQLite3('TaskList.db');
      $db->busyTimeout(5000);

      $query = 'SELECT * FROM '.$name.' WHERE rowid='.$item_id.';';
      $res = $db->query($query);

      if ($row = $res->fetchArray()) {
         $size = $row['size'];
         $data = pack("a".$size, $row['item']);
      } else {
         return fail("GetItem() -> db query failed : "+$query);
      }

      $db->close();
      unset($db);

      $name_len = strlen($name);
      $name_raw = pack("a".$name_len, $name);

      $header = pack("A4LLLLA4", "CpH1", _ITEM, $item_id, $name_len, $size, "z2C3");
      return $header.$name_raw.$data;
   }
   //--------------------------------------------------------------------
   function DeleteBatch($name, $count, $body) {
      $db = new SQLite3('TaskList.db');
      $db->busyTimeout(5000);

      if ($count <= 0) http_response_code(500);

      $items = unpack("l".$count, $body);

      for ($i = $j = 0; $i < $count; $i++) {
         $check = $db->exec("DELETE FROM ".$name." WHERE rowid = ".$items[$i+1]);
         if ($check) {
            $gone[$j++] = $items[$i+1];
         }
      }
      $db->close();
      unset($db);

      $name_len = strlen($name);
      $name_raw = pack("a".$name_len, $name);
      
      $data_len = $j*4;

      $header = pack("A4LLLLA4", "CpH1", _DELETE, $j, $name_len, $data_len, "z2C3");
      if ($data_len > 0) {
         $data = "";
         for ($i = 0; $i < $j; $i++) $data .= pack("l", $gone[$i]);
         return $header.$name_raw.$data;
      }

      return $header.$name_raw;
   }
   //--------------------------------------------------------------------
   function InsertItem($name, $item) {
      $db = new SQLite3('TaskList.db');
      $db->busyTimeout(5000);

      $check = $db->query("SELECT name FROM sqlite_master WHERE name='".$name."'");
      if ($check->fetchArray() === false) {
         $db->exec("CREATE TABLE ".$name." (item TEXT NOT NULL, size INTEGER, start REAL, target REAL, modified REAL)");
      }

      $name_len = strlen($name);
      $item_len = strlen($item);
      $check = $db->exec("INSERT INTO ".$name."(item, size, start, target, modified) VALUES('".$item."',".$item_len.", julianday('now'), julianday('now'), julianday('now'))");
      if ($check) {
         $header = pack("A4LLLLA4", "CpH1", _NEW_ITEM, $db->lastInsertRowID(), $name_len, $item_len, "z2C3");
         $name_raw = pack("a".$name_len, $name);
         $data = pack("a".$item_len, $item);
         return $header.$name_raw.$data;
      }
                                                                  
      $db->close();
      unset($db);

      return pack("A4LLLLA4", "CpH1", _ERROR, 0, 0, 0, "z2C3");
   };
   //--------------------------------------------------------------------
   function UpdateItem($name, $item_id, $item) {
      $db = new SQLite3('TaskList.db');
      $db->busyTimeout(5000);

      $name_len = strlen($name);
      $item_len = strlen($item);

      $check = $db->exec("UPDATE ".$name." SET item = '".$item."', size = ".$item_len.", modified = julianday('now') WHERE rowid = ".$item_id.";");
      if ($check) {
         $header = pack("A4LLLLA4", "CpH1", _UPDATE, $item_id, $name_len, $item_len, "z2C3");
         $name_raw = pack("a".$name_len, $name);
         $data = pack("a".$item_len, $item);
         return $header.$name_raw.$data;
      }
                                                                  
      $db->close();
      unset($db);

      return pack("A4LLLLA4", "CpH1", _ERROR, 0, 0, 0, "z2C3");
   };
   //--------------------------------------------------------------------

   if (!($f = fopen("php://input", "rb")) ) fail("can't open php input stream");
   if ( !($raw_header = fread($f, 24)) ) fail("fail at reading header");

   $header_format = 'A4magic1/L1cmd/L1param/L1name_len/l1body_len/A4magic2';
   $header = unpack($header_format, $raw_header);

   if ( !($name_raw = fread($f, $header['name_len'])) ) fail("fail at reading name");
   $name = unpack("a".$header['name_len'], $name_raw)[1];

   if ($header['body_len'] != 0) {
      $body_raw = fread($f, $header['body_len']);
   } else {
      $body_raw = "";
   }

   switch($header['cmd']) {
      case _GET_INDEX:
         echo GetIndex($name);
         break;
      case _GET_ITEM:
         echo GetItem($name, $header['param']);
         break;
      case _INSERT:
         $body = unpack("a".$header['body_len'], $body_raw)[1];
         echo InsertItem($name, $body);
         break;
      case _DELETE:
         $count = $header['param'];
         echo DeleteBatch($name, $count, $body_raw);
         break;
      case _UPDATE:
         $body = unpack("a".$header['body_len'], $body_raw)[1];
         echo UpdateItem($name, $header['param'], $body);
         break;
      default:
         fail("unsupported command");
         break;
   }

   fclose($f);


?>

