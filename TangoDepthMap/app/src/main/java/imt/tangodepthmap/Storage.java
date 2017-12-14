package imt.tangodepthmap;

import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


/**
 * Created by Flavien on 14/12/2017.
 *
 */

public class Storage
{

    public static final String FOLDER_NAME = Environment.DIRECTORY_DOWNLOADS;

    private static final String FILE_NAME_ROOT = "Recording ";

    private static final String FILE_FORMAT = ".mosbi";

    private static final String FILE_NAME_REGEXP =  "^" + Storage.FILE_NAME_ROOT +
                                                    "([0-9]+)" + Storage.FILE_FORMAT + "$";

    private static final Pattern FILE_NAME_REGEXP_PATTERN =  Pattern.compile(Storage.FILE_NAME_REGEXP);

    public static String getFilePath(File folder) {
        String folderPath = Storage.getFolderPath(folder);
        String file = Storage.getFile(folderPath);
        return folderPath + "/" + file;
    }

    private static String getFolderPath(File folder) {
        return folder.getAbsolutePath();
    }

    private static String getFile(String folderPath) {
        File[] files = new File(folderPath).listFiles();
        Integer max = 0;

        for (File file : files) {
            if (file.isFile()) {
                String name = file.getName();
                if (name.matches(Storage.FILE_NAME_REGEXP)) {
                    Matcher m = Storage.FILE_NAME_REGEXP_PATTERN.matcher(name);
                    if (m.find()) {
                        Integer fileNumber = Integer.parseInt(m.group(1));
                        if(max < fileNumber) {
                            max = fileNumber;
                        }
                    }
                }
            }
        }
        max++;
        return Storage.FILE_NAME_ROOT + max.toString() + Storage.FILE_FORMAT;
    }

}
