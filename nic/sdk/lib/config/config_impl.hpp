#include <sys/stat.h>

namespace sdk {
namespace lib {

template <typename T>
T
config::get(const std::string &key) {
    T data;

    // sync between threads (across both read and write) in the process.
    // needed this on top of the below flock(), else any outstanding (exclusive)
    // lock on the file from this process (writing thread i.e. set())
    // would get automatically covereted to lock of the type we are requesting
    // here in this thread here.
    this->conf_mtx_.lock();
    // sync across processes.
    if (flock(this->conf_fd_, LOCK_SH) < 0) {
        SDK_TRACE_ERR("Failed to read lock file %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
        this->conf_mtx_.unlock();
        return "";
    }

    if (update_ptree_()) {  // reread file if got updated since last read.
        data = this->prop_tree_.get<T>(key);
    }

    if (flock(this->conf_fd_, LOCK_UN) < 0) {
        SDK_TRACE_ERR("Failed to release read lock on %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
    }
    this->conf_mtx_.unlock();
    return data;
}

template <typename T>
T
config::get(const std::string &key, T &default_data) {
    T data;

    this->conf_mtx_.lock();
    // sync across processes.
    if (flock(this->conf_fd_, LOCK_SH) < 0) {
        SDK_TRACE_ERR("Failed to read lock file %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
        this->conf_mtx_.unlock();
        return "";
    }

    if (update_ptree_()) {  // reread file if got updated since last read.
        data = this->prop_tree_.get<T>(key, default_data);
    }

    if (flock(this->conf_fd_, LOCK_UN) < 0) {
        SDK_TRACE_ERR("Failed to release read lock on %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
    }
    this->conf_mtx_.unlock();
    return data;
}

template <typename T>
sdk_ret_t
config::set(const std::string &key, const T &data) {
    sdk_ret_t ret;
    struct stat sb;

    // sync. between threads (across both read and write) in the process.
    // needed, else any existing shared flock on the file from this process
    // would get converted automaticallly to the exclusive lock we are
    // requesting below - and with potentical context switch during the
    // conversion.
    this->conf_mtx_.lock();

    // sync. across processes.
    if (flock(this->conf_fd_, LOCK_EX) < 0) {
        SDK_TRACE_ERR("Failed to write lock %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
        this->conf_mtx_.unlock();
        return SDK_RET_ERR;
    }

    ret = SDK_RET_OK;
    if (update_ptree_()) { // reread the file, if updated since last read.
        try {
            //
            // TODO: how to sanity check for invalid json key insertion???
            // without the check, the below write_json() will fail and
            // reset the file content.
            //
            prop_tree_.put<T>(key, data);
            boost::property_tree::write_json(conf_file_, prop_tree_);
            if (fstat(this->conf_fd_, &sb) == 0) {
                this->ts_.tv_sec = sb.st_mtim.tv_sec;
                this->ts_.tv_nsec = sb.st_mtim.tv_nsec;
            } else {
                SDK_TRACE_ERR("Failed to stat file %s, err %s",
                              this->conf_file_.c_str(), strerror(errno));
            }
        } catch (std::exception const &ex) {
            ret = SDK_RET_ERR;
            SDK_TRACE_ERR("Failed to write %s to %s err %s",
                          key.c_str(), this->conf_file_.c_str(),
                          ex.what());
        }
    } else {
        ret = SDK_RET_ERR;
    }

    if (flock(this->conf_fd_, LOCK_UN) < 0) {
        SDK_TRACE_ERR("Failed to release write lock on %s, err %s",
                      this->conf_file_.c_str(), strerror(errno));
    }
    this->conf_mtx_.unlock();
    return (ret);
}

}    // namespace lib
}    // namespace sdk