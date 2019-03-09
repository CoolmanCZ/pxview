pxlib.3: pxlib.sgml
	docbook2man $<
	mv PXLIB.3 pxlib.3

%.3: %.sgml
	$(DOC_TO_MAN) $< > $@

%.html: %.sgml
	docbook2html $<
	mv index.html $@

distclean: clean
	rm -f Makefile

clean:
	rm -f ${man_MANS} ${man_HTMLS}

SGML_FILES = PX_get_majorversion.sgml \
             PX_get_minorversion.sgml \
						 PX_get_subminorversion.sgml \
						 PX_new3.sgml \
						 PX_new2.sgml \
						 PX_new.sgml \
						 PX_open_file.sgml \
						 PX_open_fp.sgml \
						 PX_open_gsf.sgml \
						 PX_create_file.sgml \
						 PX_create_blob_file.sgml \
						 PX_create_fp.sgml \
						 PX_get_data_short.sgml \
						 PX_get_data_long.sgml \
						 PX_get_data_double.sgml \
						 PX_get_data_alpha.sgml \
						 PX_get_data_byte.sgml \
						 PX_get_data_bytes.sgml \
						 PX_get_data_bcd.sgml \
						 PX_get_data_blob.sgml \
						 PX_get_data_graphic.sgml \
						 PX_get_opaque.sgml \
						 PX_put_data_short.sgml \
						 PX_put_data_long.sgml \
						 PX_put_data_double.sgml \
						 PX_put_data_alpha.sgml \
						 PX_put_data_byte.sgml \
						 PX_put_data_bytes.sgml \
						 PX_put_data_bcd.sgml \
						 PX_get_record.sgml \
						 PX_get_record2.sgml \
						 PX_put_record.sgml \
						 PX_put_recordn.sgml \
						 PX_insert_record.sgml \
						 PX_update_record.sgml \
						 PX_delete_record.sgml \
						 PX_retrieve_record.sgml \
						 PX_get_field.sgml \
						 PX_get_fields.sgml \
						 PX_get_num_fields.sgml \
						 PX_get_num_records.sgml \
						 PX_get_recordsize.sgml \
						 PX_add_primary_index.sgml \
						 PX_read_primary_index.sgml \
						 PX_write_primary_index.sgml \
						 PX_new_blob.sgml \
						 PX_open_blob_file.sgml \
						 PX_open_blob_fp.sgml \
						 PX_close_blob.sgml \
						 PX_read_blobdata.sgml \
						 PX_set_blob_file.sgml \
						 PX_set_targetencoding.sgml \
						 PX_set_inputencoding.sgml \
						 PX_set_tablename.sgml \
						 PX_set_parameter.sgml \
						 PX_get_parameter.sgml \
						 PX_set_value.sgml \
						 PX_get_value.sgml \
						 PX_close.sgml \
						 PX_delete.sgml \
						 PX_timestamp2string.sgml \
						 PX_time2string.sgml \
						 PX_date2string.sgml \
						 PX_strdup.sgml

EXTRA_DIST = ${SGML_FILES} pxformat.txt paradox4.txt pxlib.sgml

man_MANS = $(SGML_FILES:.sgml=.3) pxlib.3
man_HTMLS = $(SGML_FILES:.sgml=.html) pxlib.html

html: $(SGML_FILES:.sgml=.html) pxlib.html

.PHONY: html

