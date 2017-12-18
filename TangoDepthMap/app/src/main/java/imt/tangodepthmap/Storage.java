package imt.tangodepthmap;

import android.util.Log;

import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


/**
 * Created by Flavien DELANGLE and Maxime SEURRE on 14/12/2017.
 * Class to manage the storage paths of our recordings
 */

public abstract class Storage
{
    private static String elementPath;

    private static final String TAG = "TangoDepthMapActivity";

    public static final String ROOT_DIRECTORY_NAME = "/storage/emulated/0/Movies/";

    public static final String PROJECT_DIRECTORY_NAME = "DepthMap_Recordings";

    private static final String ELEMENT_DIRECTORY_NAME_PREFIX = "Recording_";

    private static final String ELEMENT_DIRECTORY_REGEXP =  "^" + Storage.ELEMENT_DIRECTORY_NAME_PREFIX +
                                                    "([0-9]+)" + "$";

    private static final Pattern ELEMENT_DIRECTORY_REGEXP_PATTERN =  Pattern.compile(Storage.ELEMENT_DIRECTORY_REGEXP);


    /**
     * Create a directory for a new recording and return it's absolute path
     * Project Structure :
     * | Movies <= Storage.ROOT_DIRECTORY_NAME
     *   | DepthMap_Recordings <= Storage.PROJECT_DIRECTORY_NAME
     *     | Recording_0001
     *     | Recording_0002
     *     ...
     *   ...
     * @return path to the new recording directory
     */
    public static String getFilePath() {
        String directoryPath = Storage.getDirectoryPath();
        return Storage.getFile(directoryPath);
    }

    /**
     * Retrieve the path to the project directory (create it if it doesn't exist)
     * @return path to the project directory
     */
    private static String getDirectoryPath() {
        String projectPath = Storage.ROOT_DIRECTORY_NAME + "/" + Storage.PROJECT_DIRECTORY_NAME + "/";
        File projectDirectory = new File(projectPath);
        if(!projectDirectory.exists()) {
            Boolean success = projectDirectory.mkdirs();
            Log.d(TAG, "Project directory make succeded : " + success);
        }
        return projectDirectory.getAbsolutePath() + "/";
    }

    /**
     * Create a new directory for the current recording and retrieve it full path
     * @param directoryPath path to the project directory
     * @return path to the current recording directory
     */
    private static String getFile(String directoryPath) {
        File[] files = new File(directoryPath).listFiles();
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
        elementPath = directoryPath + Storage.ELEMENT_DIRECTORY_NAME_PREFIX + Storage.format(++max) + "/";
        Boolean success = new File(elementPath).mkdirs();
        return elementPath;
    }

    /**
     * Format the number of the recording to assure a good display order on Windows
     * Otherwise we would have the following order : Recording_1, Recording_10, ..., Recording_2, ...
     * @param number the number we want to format
     * @return a string representing the formatted number
     */
    private static String format(Integer number) {
        String result = number.toString();
        Integer amount = 4 - result.length();
        return new String(new char[amount]).replace("\0", "0") + result;
    }
}
