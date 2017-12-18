package imt.tangodepthmap;

import android.media.MediaScannerConnection;
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
    private static String elementPath;

    private static final String TAG = "TangoDepthMapActivity";

    public static final String HARDCODED_PATH = "/storage/emulated/0/Movies/";

    public static final String ROOT_DIRECTORY_NAME = Environment.DIRECTORY_MOVIES;

    public static final String PROJECT_DIRECTORY_NAME = "DepthMap_Recordings";

    private static final String ELEMENT_DIRECTORY_NAME_PREFIX = "Recording_";

    private static final String ELEMENT_DIRECTORY_REGEXP =  "^" + Storage.ELEMENT_DIRECTORY_NAME_PREFIX +
                                                    "([0-9]+)" + "$";

    private static final Pattern ELEMENT_DIRECTORY_REGEXP_PATTERN =  Pattern.compile(Storage.ELEMENT_DIRECTORY_REGEXP);

    public static String getFilePath(File rootFolder) {
        String folderPath = Storage.getFolderPath(rootFolder);
        return Storage.getFile(folderPath);
    }

    public static String getFilePath() {
        File folder = new File(Storage.HARDCODED_PATH);
        String folderPath = Storage.getFolderPath(folder);
        return Storage.getFile(folderPath);
    }

    private static String getFolderPath(File rootFolder) {
        String projectPath = rootFolder.getAbsolutePath() + "/" + Storage.PROJECT_DIRECTORY_NAME + "/";
        File projectFolder = new File(projectPath);
        if(!projectFolder.exists()) {
            Boolean success = projectFolder.mkdirs();
            Log.d(TAG, "Project folder make succeded : " + success);
        }
        return projectFolder.getAbsolutePath() + "/";
    }

    private static String getFile(String folderPath) {
        File[] files = new File(folderPath).listFiles();
        Integer max = 0;
        for (File file : files) {
            if (file.isDirectory()) {
                String name = file.getName();
                if (name.matches(Storage.ELEMENT_DIRECTORY_REGEXP)) {
                    Matcher m = Storage.ELEMENT_DIRECTORY_REGEXP_PATTERN.matcher(name);
                    if (m.find()) {
                        Integer fileNumber = Integer.parseInt(m.group(1));
                        if(max < fileNumber) {
                            max = fileNumber;
                        }
                    }
                }
            }
        }
        elementPath = folderPath + Storage.ELEMENT_DIRECTORY_NAME_PREFIX + Storage.format(++max) + "/";
        Boolean success = new File(elementPath).mkdirs();
        return elementPath;
    }

    private static String format(Integer number) {
        String result = number.toString();
        Integer amount = 4 - result.length();
        return new String(new char[amount]).replace("\0", "0") + result;
    }
}
