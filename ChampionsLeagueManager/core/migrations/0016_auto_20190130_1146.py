# Generated by Django 2.1.4 on 2019-01-30 10:46

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('core', '0015_auto_20190125_1318'),
    ]

    operations = [
        migrations.RenameField(
            model_name='knockoutmatch',
            old_name='match',
            new_name='referred_match',
        ),
    ]
