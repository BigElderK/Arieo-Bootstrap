package com.arieo.bootstrap;

import com.google.androidgamesdk.GameActivity;
import android.os.Bundle;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Properties;
import android.app.Application;
import java.lang.reflect.Method;

public class MainActivity extends GameActivity 
{
    private static final String TAG = "ArieoEngine";
    
    // Native method to set environment variable
    public native boolean nativeSetEnvironmentVariable(String name, String value);
    public native String nativePrepareEngine(String manifestPath);
    
    static 
    {
        // System.loadLibrary("arieo_main_module");
        
        try {
            // Get application context using reflection to call getFilesDir()
            Class<?> activityThreadClass = Class.forName("android.app.ActivityThread");
            Method currentApplicationMethod = activityThreadClass.getMethod("currentApplication");
            Application application = (Application) currentApplicationMethod.invoke(null);

            if (application != null) {

            } else {
                throw new RuntimeException("Could not get application context");
            }
        } catch (Exception e) {
            // Fallback: Load our main module from APK
            System.loadLibrary("arieo_main_module");
            Log.d("ArieoEngine", "Fallback: Loaded arieo_main_module from APK due to: " + e.getMessage());
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) 
    {
        File filesDir = getFilesDir();
        String bootstrapINIPath = filesDir.getAbsolutePath() + "/engine/bootstrap.ini";
        Log.d(TAG, "Bootstrap INI path: " + bootstrapINIPath);

        // Load LIBCXX_PATH and BOOTSTRAP_PATH from bootstrap.ini
        {
            //[bootstrap.ini]
            //LIBCXX_PATH=/data/data/com.arieo.bootstrap/files/engine/Arieo-Bootstrap/android.armv8/bin/RelWithDebInfo/libc++_shared.so
            //BOOTSTRAP_PATH=/data/data/com.arieo.bootstrap/files/engine/Arieo-Bootstrap/android.armv8/bin/RelWithDebInfo/libarieo_bootstrap.so

            //Root path: /data/user/0/... vs /data/data/... (these are actually the same directory on Android — /data/data is a symlink to /data/user/0)
            // Deploy: /data/data/com.arieo.bootstrap/files
            // Runtime: /data/user/0/com.arieo.bootstrap/files
            Properties bootstrapIni = loadBootstrapIni(bootstrapINIPath);
            if (bootstrapIni == null) {
                Log.e(TAG, "Failed to load bootstrap.ini from: " + bootstrapINIPath);
                return;
            }

            String libCXXPath = bootstrapIni.getProperty("LIBCXX_PATH");
            String bootstrapModulePath = bootstrapIni.getProperty("BOOTSTRAP_PATH");

            if (libCXXPath == null || libCXXPath.isEmpty()) {
                Log.e(TAG, "LIBCXX_PATH not found in bootstrap.ini");
                return;
            }
            if (bootstrapModulePath == null || bootstrapModulePath.isEmpty()) {
                Log.e(TAG, "BOOTSTRAP_PATH not found in bootstrap.ini");
                return;
            }

            Log.d(TAG, "LIBCXX_PATH: " + libCXXPath);
            Log.d(TAG, "BOOTSTRAP_PATH: " + bootstrapModulePath);

            System.load(libCXXPath);
            Log.d(TAG, "Loaded libc++_shared.so: " + libCXXPath);

            System.load(bootstrapModulePath);
            Log.d(TAG, "Loaded arieo_bootstrap: " + bootstrapModulePath);
        }


        // String mainModulePath = filesDir.getAbsolutePath() + "/engine/Arieo-Module-Main/android.armv8/bin/RelWithDebInfo/libarieo_main_module.so";
        // System.load(mainModulePath);
        // Log.d("ArieoEngine", "Loaded arieo_main_module from files directory: " + mainModulePath);

        // Load manifest path
        String applicationManifestFilePath = null;
        {
            String obbFolder = getObbFolder();
            Log.d(TAG, "OBB folder: " + obbFolder);

            String applicationManifest = getArieoApplicationManifest();
            Log.d(TAG, "Raw application manifest: " + applicationManifest);

            // Replace the placeholder with actual OBB folder path:
            applicationManifestFilePath = applicationManifest; // Gets "${obb_folder}/arieo_render_test_app"
            if (applicationManifestFilePath != null && applicationManifestFilePath.contains("$${obb_folder}")) {
                applicationManifestFilePath = applicationManifestFilePath.replace("$${obb_folder}", obbFolder);
                Log.d(TAG, "Replaced manifest value: " + applicationManifestFilePath);
            }

            if(applicationManifestFilePath == null)
            {
                Log.e(TAG, "Failed to load application manifest file path from meta-data");
                return;
            }

            if(!new File(applicationManifestFilePath).exists())
            {
                Log.e(TAG, "Application manifest file does not exist at path: " + applicationManifestFilePath);
                return;
            }
        }

        // Set real system environment variable using native JNI call
        // Set APP_MANIFEST_PATH and APP_MANIFEST_PATH_DIR environment variables so native code can access them
        {
            Log.d(TAG, "Setting APP_MANIFEST_PATH environment variable to: " + applicationManifestFilePath);
            try {
                boolean success = nativeSetEnvironmentVariable("APP_MANIFEST_PATH", applicationManifestFilePath)
                && nativeSetEnvironmentVariable("APP_MANIFEST_DIR", new File(applicationManifestFilePath).getParent());
                
                if (success) {
                    Log.d(TAG, "Successfully set APP_MANIFEST_PATH environment variable via JNI");
                    // Verify by reading it back from system
                    String verifyValue = System.getenv("APP_MANIFEST_PATH");
                    Log.d(TAG, "Verification - APP_MANIFEST_PATH = " + verifyValue);
                } else {
                    Log.e(TAG, "JNI call failed to set APP_MANIFEST_PATH environment variable");
                }
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "JNI method nativeSetEnvironmentVariable not found", e);
            }
        }

        // Set app internal data directory as environment variable - using internal files dir to avoid namespace issues
        {
            String appInternalDataDir = getFilesDir().getAbsolutePath();
            Log.d(TAG, "Setting APP_INTERNAL_DATA_DIR environment variable to: " + appInternalDataDir);
            try {
                boolean success = nativeSetEnvironmentVariable("APP_INTERNAL_DATA_DIR", appInternalDataDir);
                if (success) {
                    Log.d(TAG, "Successfully set APP_DATA_DIR environment variable via JNI");
                } else {
                    Log.e(TAG, "JNI call failed to set APP_INTERNAL_DATA_DIR environment variable");
                }
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "JNI method nativeSetEnvironmentVariable not found", e);
            }
        }

        {
            // Set app external data directory as environment variable - using internal files dir to avoid namespace issues
            String appExternalDataDir = getExternalFilesDir(null).getAbsolutePath();
            Log.d(TAG, "Setting APP_EXTERNAL_DATA_DIR environment variable to: " + appExternalDataDir);
            try {
                boolean success = nativeSetEnvironmentVariable("APP_EXTERNAL_DATA_DIR", appExternalDataDir);
                if (success) {
                    Log.d(TAG, "Successfully set APP_EXTERNAL_DATA_DIR environment variable via JNI");
                } else {
                    Log.e(TAG, "JNI call failed to set APP_EXTERNAL_DATA_DIR environment variable");
                }
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "JNI method nativeSetEnvironmentVariable not found", e);
            }
        }

        // Load Manifest file and get main module path from it, then load the main module
        String mainModulePath = nativePrepareEngine(applicationManifestFilePath);    
        if(mainModulePath != null) {
            System.load(mainModulePath);
            Log.d(TAG, "Loaded main module from manifest path: " + mainModulePath);
        } else {
            Log.e(TAG, "Failed to prepare engine with manifest file path: " + applicationManifestFilePath);
        }

        // Load manifest_file
        super.onCreate(savedInstanceState);

        // Load arieo.application.manifest from manifest meta-data
        Log.d(TAG, "MainActivity onCreate() called");
    }
    
    private Properties loadBootstrapIni(String path) {
        Properties props = new Properties();
        File file = new File(path);
        if (!file.exists()) {
            Log.e(TAG, "bootstrap.ini not found at: " + path);
            return null;
        }
        try (FileInputStream fis = new FileInputStream(file)) {
            props.load(fis);
            Log.d(TAG, "Loaded bootstrap.ini: " + props.toString());
            return props;
        } catch (IOException e) {
            Log.e(TAG, "Failed to read bootstrap.ini: " + e.getMessage());
            return null;
        }
    }

    private String getArieoApplicationManifest() {
        try {
            ApplicationInfo appInfo = getPackageManager().getApplicationInfo(
                getPackageName(), PackageManager.GET_META_DATA);
            
            if (appInfo.metaData != null) {
                return appInfo.metaData.getString("arieo.application.manifest");
            }
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Package name not found", e);
        } catch (Exception e) {
            Log.e(TAG, "Error reading meta-data", e);
        }
        return null;
    }
    
    private String getObbFolder() {
        try {
            // Get the main OBB directory for this app
            File obbDir = getObbDir();
            if (obbDir != null && obbDir.exists()) {
                // Return the OBB directory path
                String obbDirPath = obbDir.getAbsolutePath();
                Log.d(TAG, "Found OBB directory: " + obbDirPath);
                return obbDirPath;
            }
            
            // Fallback: construct expected OBB folder path
            File externalStorage = Environment.getExternalStorageDirectory();
            File obbFolder = new File(externalStorage, "Android/obb/" + getPackageName());
            
            // Return the OBB folder path (create if doesn't exist)
            String obbFolderPath = obbFolder.getAbsolutePath();
            Log.d(TAG, "Using OBB folder path: " + obbFolderPath);
            return obbFolderPath;
            
        } catch (Exception e) {
            Log.e(TAG, "Error getting OBB folder path", e);
            return null;
        }
    }
}