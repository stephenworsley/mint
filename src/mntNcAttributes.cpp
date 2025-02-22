#include "mntLogger.h"
#include "mntNcAttributes.h"
#include <cstdio>
#include <cstring>
#include <netcdf.h>
#include <iostream>

LIBRARY_API
int mnt_ncattributes_new(NcAttributes_t** self) {
  *self = new NcAttributes_t();
  return 0;
}

LIBRARY_API
int mnt_ncattributes_del(NcAttributes_t** self) {
  (*self)->attStr.clear();
  (*self)->attInt.clear();
  (*self)->attDbl.clear();
  delete *self;
  return 0;
}

LIBRARY_API
int mnt_ncattributes_read(NcAttributes_t** self, int ncid, int varid) {

  int ier;

  // inquire about the variable
  int natts = 0;
  ier = nc_inq_var(ncid, varid, NULL, NULL, NULL, NULL, &natts);
  if (ier != NC_NOERR) {
    std::string msg = "could not inquire about variable with Id " + std::to_string(varid);
    mntlog::error(__FILE__, __func__, __LINE__, msg);
    return 3;
  }

  // get the attributes
  char attname[NC_MAX_NAME + 1];
  std::size_t n;
  nc_type xtype;
  for (int i = 0; i < natts; ++i) {
    ier = nc_inq_attname(ncid, varid, i, attname);
    ier = nc_inq_att(ncid, varid, attname, &xtype, &n);
    if (n == 1 && xtype == NC_DOUBLE) {
      double val;
      ier = nc_get_att_double(ncid, varid, attname, &val);
      (*self)->attDbl.insert(std::pair<std::string, double>(std::string(attname), val));
    }
    else if (n == 1 && xtype == NC_INT) {
      int val;
      ier = nc_get_att_int(ncid, varid, attname, &val);
      (*self)->attInt.insert(std::pair<std::string, int>(std::string(attname), val));
    }
    else if (xtype == NC_CHAR) {
      std::string val(n, ' ');
      ier = nc_get_att_text(ncid, varid, attname, &val[0]);
      (*self)->attStr.insert(std::pair<std::string, std::string>(std::string(attname), val.c_str()));
    }
    else {
      std::string msg = "unsupported attribute type " + 
                        std::to_string(xtype) + " of length " + std::to_string(n);
      mntlog::warn(__FILE__, __func__, __LINE__, msg);
    }
    if (ier != NC_NOERR) {
      std::string msg = "failed to read attribute " + std::string(attname) + 
                        " of variable with Id " + std::to_string(varid);
      mntlog::warn(__FILE__, __func__, __LINE__, msg);
    }
  }

  return 0;
}

LIBRARY_API
int mnt_ncattributes_write(NcAttributes_t** self, int ncid, int varid) {

  int ier;

  for (auto it = (*self)->attStr.cbegin(); it != (*self)->attStr.cend(); ++it) {
    ier = nc_put_att_text(ncid, varid, 
                          it->first.c_str(), it->second.size(), it->second.c_str());
    if (ier != NC_NOERR) {
      std::string msg = "could not put attribute " + it->first + " = " + it->second;
      mntlog::error(__FILE__, __func__, __LINE__, msg);
      return 4;
    }

  }
  for (auto it = (*self)->attInt.cbegin(); it != (*self)->attInt.cend(); ++it) {
    ier = nc_put_att_int(ncid, varid, 
                         it->first.c_str(), NC_INT, 1, &it->second);
    if (ier != NC_NOERR) {
      std::string msg = "could not put attribute " + it->first + " = " + 
                        std::to_string(it->second);
      mntlog::error(__FILE__, __func__, __LINE__, msg);
      return 5;
    }
  }
  for (auto it = (*self)->attDbl.cbegin(); it != (*self)->attDbl.cend(); ++it) {
    ier = nc_put_att_double(ncid, varid, 
                            it->first.c_str(), NC_DOUBLE, 1, &it->second);
    if (ier != NC_NOERR) {
      std::string msg = "could not put attribute " + it->first + " = " + 
                        std::to_string(it->second);
      mntlog::error(__FILE__, __func__, __LINE__, msg);
      return 6;
    }
  }

  return 0;
}

LIBRARY_API
int mnt_ncattributes_isIntensive(NcAttributes_t** self) {
  if ((*self)->attStr["field_methods"] == "intensive") {
    return 1;
  }
  return 0;
}

LIBRARY_API
int mnt_ncattributes_print(NcAttributes_t** self) {

  std::cout << "string attributes:\n";
  for (auto it = (*self)->attStr.cbegin(); it != (*self)->attStr.cend(); ++it) {
    std::cout << it->first << " -> \"" << it->second << "\"\n";
  }
  std::cout << "int attributes   :\n";
  for (auto it = (*self)->attInt.cbegin(); it != (*self)->attInt.cend(); ++it) {
    std::cout << it->first << " -> \"" << it->second << "\"\n";
  }
  std::cout << "double attributes:\n";
  for (auto it = (*self)->attDbl.cbegin(); it != (*self)->attDbl.cend(); ++it) {
    std::cout << it->first << " -> \"" << it->second << "\"\n";
  }

  return 0;
}

