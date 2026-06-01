package com.cryptoscanner.storage

import android.content.Context
import android.content.SharedPreferences
import android.util.Log
import androidx.room.Dao
import androidx.room.Database
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.sqlite.db.SupportSQLiteOpenHelper
import androidx.sqlite.db.framework.FrameworkSQLiteOpenHelperFactory
import com.cryptoscanner.model.WalletResult
import kotlinx.coroutines.flow.Flow
import net.sqlcipher.database.SQLiteDatabase
import net.sqlcipher.database.SupportFactory

// ---------------------------------------------------------------------------
// DAO
// ---------------------------------------------------------------------------

@Dao
interface WalletResultDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insert(result: WalletResult): Long

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertAll(results: List<WalletResult>)

    @Query("SELECT * FROM wallet_results ORDER BY timestamp DESC")
    fun getAllResults(): Flow<List<WalletResult>>

    @Query("SELECT * FROM wallet_results WHERE network = :network ORDER BY timestamp DESC")
    fun getResultsByNetwork(network: String): Flow<List<WalletResult>>

    @Query("SELECT COUNT(*) FROM wallet_results")
    suspend fun getCount(): Int

    @Query("DELETE FROM wallet_results")
    suspend fun deleteAll()

    @Query("DELETE FROM wallet_results WHERE id = :id")
    suspend fun deleteById(id: Long)
}

// ---------------------------------------------------------------------------
// Room Database
// ---------------------------------------------------------------------------

@Database(
    entities = [WalletResult::class],
    version = 1,
    exportSchema = false
)
abstract class WalletDatabase : RoomDatabase() {
    abstract fun walletResultDao(): WalletResultDao

    companion object {
        private const val TAG = "WalletDatabase"
        private const val DB_NAME = "cryptoscanner.db"

        @Volatile
        private var INSTANCE: WalletDatabase? = null

        /**
         * Returns (or creates) the singleton [WalletDatabase] instance.
         * The database is encrypted with SQLCipher using a passphrase derived
         * from an Android Keystore-protected key.
         */
        fun getInstance(context: Context): WalletDatabase {
            return INSTANCE ?: synchronized(this) {
                INSTANCE ?: buildDatabase(context.applicationContext).also {
                    INSTANCE = it
                }
            }
        }

        private fun buildDatabase(context: Context): WalletDatabase {
            val passphrase = WalletDatabaseHelper.getOrCreatePassphrase(context)
            val passphraseBytes = passphrase.toByteArray(Charsets.UTF_8)
            val factory = SupportFactory(passphraseBytes)

            Log.d(TAG, "Building encrypted Room database with SQLCipher")
            return Room.databaseBuilder(context, WalletDatabase::class.java, DB_NAME)
                .openHelperFactory(factory)
                .fallbackToDestructiveMigration()
                .build()
        }
    }
}

// ---------------------------------------------------------------------------
// Passphrase Manager
// ---------------------------------------------------------------------------

/**
 * Manages the SQLCipher database passphrase.
 *
 * Strategy:
 *  1. On first launch, generate a random 32-byte key, encrypt it with the
 *     Android Keystore key, and persist the encrypted form in SharedPreferences.
 *  2. On subsequent launches, load from SharedPreferences and decrypt using
 *     the Keystore key.
 *
 * The plaintext passphrase is kept only in memory for the duration it is needed.
 */
object WalletDatabaseHelper {

    private const val PREFS_NAME = "cryptoscanner_secure_prefs"
    private const val KEY_DB_PASSPHRASE = "db_passphrase_encrypted"

    /**
     * Returns the plaintext database passphrase, creating and storing it on
     * first call.
     */
    fun getOrCreatePassphrase(context: Context): String {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val existing = prefs.getString(KEY_DB_PASSPHRASE, null)

        return if (existing != null) {
            // Decrypt the stored passphrase
            KeystoreManager.decrypt(existing)
        } else {
            // Generate a new passphrase, encrypt, and persist
            val encrypted = KeystoreManager.generateAndEncryptRandomKey()
            prefs.edit().putString(KEY_DB_PASSPHRASE, encrypted).apply()
            KeystoreManager.decrypt(encrypted)
        }
    }

    /**
     * Wipes the stored passphrase from SharedPreferences.
     * After calling this, the database will be inaccessible unless the
     * plaintext passphrase is known.
     *
     * Use only when the user explicitly requests a full data wipe.
     */
    fun clearPassphrase(context: Context) {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs.edit().remove(KEY_DB_PASSPHRASE).apply()
    }
}
